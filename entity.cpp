


class BitVector {
public:
    bool is_set(u32 index) const {
        u32 word_index = index >> 5;
        if (word_index >= words.size())
            return false;
        return words[word_index] & (1 << (index & 31));
    }

    void set(u32 index, bool value) {
        u32 word_index = index >> 5;
        if (word_index >= words.size()) {
            u32 new_size = words.size();
            if (new_size < 8)
                new_size = 8;
            while (new_size <= word_index)
                new_size += new_size >> 1;
            words.resize(new_size);
        }
        if (value)
            words[word_index] |= (1 << (index & 31));
        else
            words[word_index] &= ~(1 << (index & 31));
    }

    void clear() {
        words.clear();
    }

private:
    vector<u32> words;
};


struct EntityId {
    u32 index;

    operator u32() const { return index; }
};


class EntityManager {
public:
    EntityManager() : alloc_counter(0) {

    }

    EntityId alloc() {
        EntityId id;
        if (freelist.empty()) {
            id = EntityId { ++alloc_counter };
        } else {
            id = freelist.back();
            freelist.pop_back();
        }
        alive_entities.set(id.index, true);
        return id;
    }

    void free(EntityId id) {
        alive_entities.set(id.index, false);
        freelist.push_back(id);
    }

    bool is_alive(EntityId id) {
        return alive_entities.is_set(id.index);
    }

private:
    BitVector alive_entities;
    vector<EntityId> freelist;
    u32 alloc_counter;
};



template<class T>
class DenseComponentList {

};










