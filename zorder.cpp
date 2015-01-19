

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


// convert the real valued argument to a discrete representation
// in the lower 16 bits of the result
inline u32 discretize_coord(f32 coord) {
    return (u32)max(0, min(0xffff, (i32)coord));
}


static void make_index(const vector<v2> &points, vector<u32> &zindex) {
    for (v2 p : points) {
        u32 x = discretize_coord(p.x);
        u32 y = discretize_coord(p.y);
        u32 z = interleave(x, y);
        zindex.push_back(z);
    }
    std::sort(zindex.begin(), zindex.end());
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


static void debug_draw_range(pair<u32,u32> r) {
    u32 zprev = r.first;
    for (u32 z = zprev + 1; z <= r.second; zprev = z++) {
        u32 x0 = deinterleave_x(zprev);
        u32 y0 = deinterleave_y(zprev);
        u32 x1 = deinterleave_x(z);
        u32 y1 = deinterleave_y(z);
        SDL_RenderDrawLine(renderer, x0*10, y0*10, x1*10, y1*10);
    }
}


static void area_lookup(const vector<u32> &zindex, v2 p0, v2 p1, vector<v2> &result) {
    u32 xmin = discretize_coord(p0.x);
    u32 ymin = discretize_coord(p0.y);
    u32 xmax = discretize_coord(p1.x);
    u32 ymax = discretize_coord(p1.y);
    if (xmax < xmin) swap(xmin, xmax);
    if (ymax < ymin) swap(ymin, ymax);
    assert(xmin <= xmax);
    assert(ymin <= ymax);

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    debug_draw_range(make_pair(interleave(xmin, ymin), interleave(xmax, ymax)));

    vector<pair<u32,u32>> ranges;
    partition_range(xmin, ymin, xmax, ymax, ranges);
    std::sort(ranges.begin(), ranges.end());
    printf("range count: %d\n", (int)ranges.size());

    assert(!ranges.empty());
    assert(ranges[0].second >= ranges[0].first);
    for (u32 i = 1; i < ranges.size(); ++i) {
        assert(ranges[i].second >= ranges[i].first);
        assert(ranges[i].first > ranges[i-1].second);
    }

    u32 c = 21;
    auto zsearch_start = zindex.begin();
    auto zsearch_end = std::upper_bound(zindex.begin(), zindex.end(), ranges.back().second);

    for (auto r : ranges) {
        //if (zsearch_start == zsearch_end)
        //    break;

        c += 21;
        SDL_SetRenderDrawColor(renderer, c, c, c, 255);
        debug_draw_range(r);

        auto it = std::lower_bound(zsearch_start, zsearch_end, r.first);

        for (; it != zsearch_end && *it <= r.second; ++it) {
            u32 x = deinterleave_x(*it);
            u32 y = deinterleave_y(*it);
            if (x < xmin || y < ymin || x > xmax || y > ymax)
                continue;
            printf("found: %d, %d\n", x, y);
        }

        zsearch_start = it;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect;
    rect.x = xmin*10;
    rect.y = ymin*10;
    rect.w = (xmax+1)*10 - rect.x + 1;
    rect.h = (ymax+1)*10 - rect.y + 1;
    SDL_RenderDrawRect(renderer, &rect);
}




} // namespace zorder




