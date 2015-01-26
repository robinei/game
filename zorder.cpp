
namespace zorder {


// intersperses the lower 16 bits of x with zeroes
inline u32 intersperse_zeroes(u32 x) {
    x = (x | (x << 8)) & 0x00ff00ff;
    x = (x | (x << 4)) & 0x0f0f0f0f;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;
    return x;
}


// interleave the lower 16 bits of the two arguments
inline u32 interleave(u32 x, u32 y) {
    return intersperse_zeroes(x) | (intersperse_zeroes(y) << 1);
}


// extract the x part of the interleaved numbers
inline u32 deinterleave_x(u32 z) {
    z = z & 0x55555555;
    z = (z | (z >> 1)) & 0x33333333;
    z = (z | (z >> 2)) & 0x0f0f0f0f;
    z = (z | (z >> 4)) & 0x00ff00ff;
    z = (z | (z >> 8)) & 0x0000ffff;
    return z;
}


// extract the y part of the interleaved numbers
inline u32 deinterleave_y(u32 z) {
    return deinterleave_x(z >> 1);
}


// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
inline u32 highest_bit_position(u32 v) {
    static const u8 debruijn32[32] = {
        0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
        8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
    };
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return debruijn32[(v * 0x07c4acddu) >> 27];
}


// partition the range into several subranges, where each subrange contains as
// few coordinates as possible that are outside its rectangle
static void partition_range(u32 xmin, u32 ymin, u32 xmax, u32 ymax,
                            vector<pair<u32,u32>> &ranges)
{
    assert(xmin <= xmax);
    assert(ymin <= ymax);
    u32 min = interleave(xmin, ymin);
    u32 max = interleave(xmax, ymax);
    assert(min <= max);

    // don't bother subdividing if the z-range is only slightly larger than the
    // area we care about (which is the minimum bound for the z-range)
    u32 zrange = max - min + 1;
    u32 area = (xmax - xmin + 1) * (ymax - ymin + 1);
    if (zrange <= (u32)(area * 1.1) + 4) {
        ranges.push_back(make_pair(min, max));
        return;
    }

    // calculate LITMAX and BIGMIN, which are the end and start respectively,
    // of the less wasteful subranges (which we will try to partition in turn)
    u32 litmax_x, litmax_y;
    u32 bigmin_x, bigmin_y;

    u32 first_diffbit = highest_bit_position(min ^ max);
    if (first_diffbit & 1) {
        // highest differing bit is an y bit, so split along a horizontal line
        first_diffbit >>= 1;
        u32 diffmask = 0xffff >> (16 - first_diffbit - 1);
        u32 same_highbits = ~diffmask & ymin;

        litmax_x = xmax;
        litmax_y = same_highbits | (diffmask >> 1);

        bigmin_x = xmin;
        bigmin_y = litmax_y + 1;
    } else {
        // highest differing bit is an x bit, so split along a vertical line
        first_diffbit >>= 1;
        u32 diffmask = 0xffff >> (16 - first_diffbit - 1);
        u32 same_highbits = ~diffmask & xmin;
        
        litmax_x = same_highbits | (diffmask >> 1);
        litmax_y = ymax;
        
        bigmin_x = litmax_x + 1;
        bigmin_y = ymin;
    }

    partition_range(xmin, ymin, litmax_x, litmax_y, ranges);
    partition_range(bigmin_x, bigmin_y, xmax, ymax, ranges);
}



inline u32 calc_block_size(u32 size) {
    u32 block_size = 1;
    size /= 8;
    while (block_size < size)
        block_size <<= 1;
    return block_size;
}

inline u32 snap_min(u32 pos, u32 block_size) {
    return (pos / block_size) * block_size;
}

inline u32 snap_max(u32 pos, u32 block_size) {
    if (pos == 0)
        return block_size;
    u32 r = pos % block_size;
    if (r == block_size - 1)
        return pos;
    if (r == 0)
        return pos - 1;
    return (1 + pos / block_size) * block_size - 1;
}


static const f32 gridDim = 5;


struct ZOrderIndex {
    v2 minpos;
    v2 maxpos;
    v2 size;
    vector<u32> zvalues;
    vector<pair<u32,u32>> ranges;

    ZOrderIndex() {
        reset();
    }

    void reset() {
        minpos = v2{0,0};
        maxpos = v2{0,0};
        size = v2{0,0};
        zvalues.clear();
        ranges.clear();
    }

    // convert the real valued argument to a discrete representation
    // in the lower 16 bits of the result
    inline u32 discretize_x(f32 x) const {
        return (u32)(((x - minpos.x) / size.x) * 65535.0f);
    }
    // likewise for y coord
    inline u32 discretize_y(f32 y) const {
        return (u32)(((y - minpos.y) / size.y) * 65535.0f);
    }

    inline v2 from_z(u32 z) const {
        u32 x = deinterleave_x(z);
        u32 y = deinterleave_y(z);
        v2 p;
        p.x = minpos.x + size.x * ((float)x / 65535.0f);
        p.y = minpos.y + size.y * ((float)y / 65535.0f);
        return p;
    }

    inline v2 clamp(v2 p) const {
        if (p.x < minpos.x) p.x = minpos.x;
        if (p.y < minpos.y) p.y = minpos.y;
        if (p.x > maxpos.x) p.x = maxpos.x;
        if (p.y > maxpos.y) p.y = maxpos.y;
        return p;
    }

    inline bool is_size_valid() const {
        return fabs(size.x) > 0.01f && fabs(size.y) > 0.01f;
    }

    void make_index(const vector<v2> &points) {
        zvalues.clear();
        v2 min = v2 {FLT_MAX, FLT_MAX};
        v2 max = v2 {FLT_MIN, FLT_MIN};
        for (v2 p : points) {
            if (p.x < min.x) min.x = p.x;
            if (p.y < min.y) min.y = p.y;
            if (p.x > max.x) max.x = p.x;
            if (p.y > max.y) max.y = p.y;
        }
        minpos = min;
        maxpos = max;
        size = max - min;
        if (!is_size_valid()) {
            puts("invalid size!");
            return;
        }
        for (v2 p : points) {
            u32 x = discretize_x(p.x);
            u32 y = discretize_y(p.y);
            u32 z = interleave(x, y);
            zvalues.push_back(z);
        }
        std::sort(zvalues.begin(), zvalues.end());
        printf("zvalues.size(): %d\n", (int)zvalues.size());
        printf("size.x: %.1f\n", size.x);
        printf("size.y: %.1f\n", size.y);
        printf("minpos.x: %.1f\n", minpos.x);
        printf("minpos.y: %.1f\n", minpos.y);
        printf("maxpos.x: %.1f\n", maxpos.x);
        printf("maxpos.y: %.1f\n", maxpos.y);
    }

    void area_lookup(v2 p0, v2 p1, vector<u32> &result) {
        result.clear();
        if (zvalues.empty() || !is_size_valid()) {
            puts("no lookup!");
            return;
        }

        p0 = clamp(p0);
        p1 = clamp(p1);
        u32 xmin = discretize_x(p0.x);
        u32 ymin = discretize_y(p0.y);
        u32 xmax = discretize_x(p1.x);
        u32 ymax = discretize_y(p1.y);
        if (xmax < xmin) swap(xmin, xmax);
        if (ymax < ymin) swap(ymin, ymax);
        assert(xmin <= xmax);
        assert(ymin <= ymax);

        printf("p0.x: %.1f\n", p0.x);
        printf("p0.y: %.1f\n", p0.y);
        printf("p1.x: %.1f\n", p1.x);
        printf("p1.y: %.1f\n", p1.y);
        printf("xmin: %u\n", xmin);
        printf("ymin: %u\n", ymin);
        printf("xmax: %u\n", xmax);
        printf("ymax: %u\n", ymax);

        /*SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        debug_draw_range(make_pair(interleave(xmin, ymin), interleave(xmax, ymax)));*/

        // snap to a suitable power of two block size (which may cause us to search a larger
        // area than necessary, but it reduces the number of ranges we need to search)
        u32 xblock = calc_block_size(xmax - xmin);
        u32 yblock = calc_block_size(ymax - ymin);
        u32 block = max(xblock, yblock);
        u32 xmin2 = snap_min(xmin, block);
        u32 ymin2 = snap_min(ymin, block);
        u32 xmax2 = snap_max(xmax, block);
        u32 ymax2 = snap_max(ymax, block);

        ranges.clear();
        partition_range(xmin2, ymin2, xmax2, ymax2, ranges);
        std::sort(ranges.begin(), ranges.end());
        printf("range count: %d, block: %u\n", (int)ranges.size(), block);

        assert(!ranges.empty());
        assert(ranges[0].second >= ranges[0].first);
        for (u32 i = 1; i < ranges.size(); ++i) {
            assert(ranges[i].second >= ranges[i].first);
            assert(ranges[i].first > ranges[i-1].second);
        }

        //u32 c = 21;
        auto zindex_begin = zvalues.begin();
        auto zsearch_start = zvalues.begin();
        auto zsearch_end = std::upper_bound(zvalues.begin(), zvalues.end(), ranges.back().second);

        for (auto r : ranges) {
            //if (zsearch_start == zsearch_end)
            //    break;

            /*c += 21;
            SDL_SetRenderDrawColor(renderer, c, c, c, 255);
            debug_draw_range(r);*/

            auto it = std::lower_bound(zsearch_start, zsearch_end, r.first);

            for (; it != zsearch_end && *it <= r.second; ++it) {
                u32 x = deinterleave_x(*it);
                if (x < xmin || x > xmax)
                    continue;
                u32 y = deinterleave_y(*it);
                if (y < ymin || y > ymax)
                    continue;
                u32 index = it - zindex_begin;
                printf("found: %u, %u (at %u)\n", x, y, index);
                result.push_back(index);
            }

            zsearch_start = it;
        }

        /*SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        debug_draw_rect(xmin, ymin, xmax, ymax);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        debug_draw_rect(orig_xmin, orig_ymin, orig_xmax, orig_ymax);*/
    }


    /*void debug_draw_range(pair<u32,u32> r) {
        u32 zprev = r.first;
        for (u32 z = zprev + 1; z <= r.second; zprev = z++) {
            v2 p0 = from_z(zprev) * gridDim;
            v2 p1 = from_z(z) * gridDim;
            SDL_RenderDrawLine(renderer, (u32)p0.x, (u32)p0.y, (u32)p1.x, (u32)p1.y);
        }
    }

    void debug_draw_rect(u32 xmin, u32 ymin, u32 xmax, u32 ymax) {
        u32 zmin = interleave(xmin, ymin);
        u32 zmax = interleave(xmax, ymax);
        v2 p0 = from_z(zmin) * gridDim;
        v2 p1 = from_z(zmax) * gridDim;
        SDL_Rect rect;
        rect.x = p0.x;
        rect.y = p0.y;
        rect.w = p1.x - p0.x;
        rect.h = p1.y - p0.y;
        SDL_RenderDrawRect(renderer, &rect);
    }*/
};



} // namespace zorder

