/*
 * Copyright Peter G. Jensen <root@petergjoel.dk>
 *  
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   ptrie.h
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 18:44
 */

#ifndef PTRIE_H
#define PTRIE_H

#include <stdint.h>
#include <assert.h>
#include <limits>
#include <stack>
#include <string.h>
#include <functional>
#include <memory>

#include "linked_bucket.h"



// direct 2lvl indexing in chunks of ~ 2^32
// takes up ~ 512k (256k on 32bit) for the index.
// when to keep data in bucket or send to heap - just make sure bucket does not overflow

namespace ptrie {
    typedef uint32_t uint;
    typedef unsigned char uchar;
    constexpr auto WIDTH = 16;
    template<typename D, typename N>
    struct ptrie_el_t {
        N _node;
        D _data;
    };

    template<typename N>
    struct ptrie_el_t<void, N> {
        N _node;
    };
    typedef std::pair<bool, size_t> returntype_t;

    struct base_t {
        uchar _path;
        uchar _type;
    };

    template<typename KEY>
    struct byte_iterator {
        static constexpr typename std::enable_if<std::has_unique_object_representations<KEY>::value, uchar&>::type access(KEY* data, size_t id)
        {
            return ((uchar*)data)[id];
        }
        
        static constexpr typename std::enable_if<std::has_unique_object_representations<KEY>::value, const uchar&>::type const_access(const KEY* data, size_t id)
        {
            return ((const uchar*)data)[id];
        }
        
        static constexpr typename std::enable_if<std::has_unique_object_representations<KEY>::value, size_t>::type element_size()
        {
            return sizeof(KEY);
        }
        
        static constexpr bool continious()
        {
            return {std::has_unique_object_representations<KEY>::value};
        }
        
        // add read_blob, write_blob
    };

#define PTRIETPL typename KEY, uint16_t HEAPBOUND, uint16_t SPLITBOUND, size_t ALLOCSIZE, typename T, typename I, bool HAS_ENTRIES
    
    template<
    typename KEY = uchar,
    uint16_t HEAPBOUND = 17,
    uint16_t SPLITBOUND = 129,
    size_t ALLOCSIZE = (1024 * 64),
    typename T = void,
    typename I = size_t,
    bool HAS_ENTRIES = false
    >
    class set {
    protected:

        struct fwdnode_t;
        struct node_t;

        static_assert(HEAPBOUND * (sizeof(fwdnode_t)/sizeof(size_t)) < std::numeric_limits<uint16_t>::max(),
                "HEAPBOUND * (sizeof(fwdnode_t)/sizeof(fwdnode_t)) should be less than 2^16");

        static_assert(SPLITBOUND < sizeof(fwdnode_t),
                "SPLITBOUND should be less than sizeof(fwdnode_t)");

        static_assert(SPLITBOUND > 3,
                "SPLITBOUND MUST BE LARGER THAN 3");

        static_assert(HEAPBOUND > sizeof(size_t),
                "HEAPBOUND MUST BE LARGER THAN sizeof(size_t)");
                
    protected:

        typedef ptrie_el_t<T, node_t*> entry_t;
        
        struct bucket_t {

            bucket_t() {
            }
            uchar _data;

            static constexpr size_t overhead(size_t count) {
                if (HAS_ENTRIES)
                    return count * (sizeof (uint16_t) + sizeof (I));
                else
                    return count * (sizeof (uint16_t));
            }

            constexpr I* entries(uint16_t count) {
                if (HAS_ENTRIES) return (I*) (data(0) + (count * (sizeof (uint16_t))));
                else return nullptr;
            }

            constexpr uchar* data(uint16_t count) {
                return ((uchar*) (&_data)) + overhead(count);
            }

            constexpr uint16_t& first(uint16_t count = 0, uint16_t index = 0) {
                return ((uint16_t*) &_data)[index];
            }

        };

        // nodes in the tree

        struct node_t : public base_t {
            uint16_t _count = 0; // bucket-counts
            uint32_t _totsize = 0; 
            fwdnode_t* _parent = nullptr;
            bucket_t* _data = nullptr; // back-pointers to data-array up to date
        };

        struct fwdnode_t : public base_t {
            base_t* _children[WIDTH];
            fwdnode_t* _parent;
        };

        std::shared_ptr<linked_bucket_t<entry_t, ALLOCSIZE>> _entries = nullptr;

        fwdnode_t _root;
    protected:

        base_t* fast_forward(const KEY* data, size_t length, fwdnode_t** tree_pos, uint& byte) const;
        bool bucket_search(const KEY* data, size_t length, node_t* node, uint& b_index, uint byte) const;

        bool best_match(const KEY* data, size_t length, fwdnode_t** tree_pos, base_t** node, uint& byte, uint& b_index) const;

        void split_node(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte);

        void split_fwd(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t byte);

        static inline uint16_t bytes(const uint16_t d) {
            if (d >= HEAPBOUND) return sizeof (uchar*);
            else return d;
        }

        void init();

        void erase(fwdnode_t* parent, node_t* node, size_t bucketid, int byte);
        bool merge_down(fwdnode_t* parent, node_t* node, int byte);
        void inject_byte(node_t* node, uchar topush, size_t totsize, std::function<uint16_t(size_t)> sizes);
        static constexpr uchar masks[4] = {
/*            static_cast <uchar>(0x80),
            static_cast <uchar>(0x40),
            static_cast <uchar>(0x20),
            static_cast <uchar>(0x10),*/
            static_cast <uchar>(0x08),
            static_cast <uchar>(0x04),
            static_cast <uchar>(0x02),
            static_cast <uchar>(0x01)
        };

    public:
        set();
        ~set();

        returntype_t insert(const KEY* data, size_t length);
        returntype_t insert(const KEY data)                      { return insert(&data, 1); }
        returntype_t insert(std::pair<const KEY*, size_t> data)  { return insert(data.first, data.second); }
        returntype_t insert(const std::vector<KEY>& data)        { return insert(data.data(), data.size()); }

        returntype_t exists(const KEY* data, size_t length) const;
        returntype_t exists(const KEY data) const                      { return exists(&data, 1); }
        returntype_t exists(std::pair<const KEY*, size_t> data) const  { return exists(data.first, data.second); }
        returntype_t exists(const std::vector<KEY>& data) const        { return exists(data.data(), data.size()); }
        
        bool         erase (const KEY* data, size_t length);
        bool         erase (const KEY data)                      { return erase(&data, 1); }
        bool         erase (std::pair<const KEY*, size_t> data)  { return erase(data.first, data.second); }
        bool         erase (const std::vector<KEY>& data)        { return erase(data.data(), data.size()); }
        
        set(set&&) = default;
        set& operator=(set&&) = default;

    };

    template<PTRIETPL>
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::~set() {
        std::stack<fwdnode_t*> stack;
        stack.push(&_root);
        while(!stack.empty())
        {
            fwdnode_t* next = stack.top();
            stack.pop();
            for(size_t i = 0; i < WIDTH; ++i)
            {
                base_t* child = next->_children[i];
                if(child != next)
                {
                    if(i > 0 && child == next->_children[i-1]) continue;
                    if(child->_type == 255)
                    {
                        stack.push((fwdnode_t*)child);
                    }
                    else
                    {
                        node_t* node = (node_t*) child;
                        // TODO: we should delete data here also!
                        delete[] (uchar*)node->_data;
                        delete node;
                    }
                }
            }
            if(&_root != next)
                delete next;
        }
        _entries = nullptr;
    }

    template<PTRIETPL>
    void set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::init()
    {
        _entries = nullptr;
        if constexpr (HAS_ENTRIES)
        {
            _entries = std::make_unique<linked_bucket_t<entry_t, ALLOCSIZE>>(1);
        }

        _root._parent = nullptr;
        _root._type = 255;
        _root._path = 0;

        size_t i = 0;
        for (; i < WIDTH; ++i) _root._children[i] = &_root;
    }

    template<PTRIETPL>
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::set()
    {
        init();
    }

    template<PTRIETPL>
    base_t*
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::fast_forward(const KEY* data, size_t length, fwdnode_t** tree_pos, uint& p_byte) const {
        fwdnode_t* t_pos = *tree_pos;

        uint16_t s = length*byte_iterator<KEY>::element_size(); // TODO remove minus to
        uchar* sc = (uchar*) & s;
        
        do {
            *tree_pos = t_pos;

            base_t* next;
            const auto byte = p_byte / 2;
            uchar nb;
            if (byte >= 2) nb = byte_iterator<KEY>::const_access(data, byte - 2);
            else nb = sc[1 - byte];
            nb = (nb >> (((p_byte+1)%2)*4)) & 0x0F;
            next = t_pos->_children[nb];

            assert(next != nullptr);
            if(next == t_pos)
            {
              return t_pos;
            } 
            else if (next->_type != 255) {
                return (node_t*) next;
            } else {
                t_pos = static_cast<fwdnode_t*> (next);
                ++p_byte;
            }
        } while (true);
        assert(false);
    }

    template<PTRIETPL>
    bool set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::bucket_search(const KEY* target, size_t length, node_t* node, uint& b_index, uint byte) const {
        // run through the stored data in the bucket, looking for matches
        // start by creating an encoding that "points" to the "unmatched"
        // part of the encoding. Notice, this is a shallow copy, no actual
        // heap-allocation happens!
        bool found = false;
        const auto size = length*byte_iterator<KEY>::element_size();
        uint16_t encsize;
        if (size > byte) {
            encsize = byte > 0 ? size - byte : size;
        } else {
            encsize = 0;
        }

        uint16_t first;
        uchar* tf = (uchar*) & first;
        if (byte <= 1) {
            first = size;
            if (byte == 1) {
                first <<= 8;
                tf[0] = byte_iterator<KEY>::const_access(target, 0);
            }
        } else {
            tf[1] = byte_iterator<KEY>::const_access(target, -2 + byte);
            if(byte - 1 < length)
                tf[0] = byte_iterator<KEY>::const_access(target, -2 + byte + 1);
            else
                tf[0] = 0;
        }

        bucket_t* bucket = node->_data;
        if (node->_count > 0) {
            size_t offset = 0;
            b_index = 0;
            uint16_t bs = 0;
            uchar* bsc = (uchar*) & bs;
            uint16_t f;
            uchar* fc = (uchar*) & f;
            for (; b_index < node->_count; ++b_index) {
                f = bucket->first(node->_count, b_index);
                if (f >= first) break;
                if (byte > 1) bs = encsize;
                else if (byte == 1) {
                    bs = size;
                    bsc[0] = fc[1];
                    bs -= 1;
                } 
                else // if byte == 0
                {
                    bsc[0] = fc[0];
                    bsc[1] = fc[1];
                }

                offset += bytes(bs);
            }

            if (b_index >= node->_count ||
                    bucket->first(node->_count, b_index) > first) return false;

            uchar* data = bucket->data(node->_count);
            for (; b_index < node->_count; ++b_index) {

                size_t b = 0;

                if (bucket->first(node->_count, b_index) > first) break;
                // first is 2 bytes, which is size of counter, the first part of the tree
                // if we reach here, things have same size

                if (encsize < HEAPBOUND) {
                    for (; b < encsize; ++b) {
                        if (data[offset + b] != byte_iterator<KEY>::const_access(target, b+byte)) break;
                    }
                    if (b == encsize) {
                        found = true;
                        break;
                    } else {
                        assert(byte_iterator<KEY>::const_access(target, b+byte) != data[offset + b]);
                        if (byte_iterator<KEY>::const_access(target, b+byte) < data[offset + b]) {
                            found = false;
                            break;
                        }
                        // else continue search
                    }
                } else {
                    uchar* ptr = *((uchar**) (&data[offset]));
                    if constexpr(byte_iterator<KEY>::continious())
                    {
                        int cmp = memcmp(ptr, &byte_iterator<KEY>::const_access(target, byte), encsize);
                        if (cmp > 0) {
                            found = false;
                            break;
                        }

                        if (cmp == 0) {
                            found = true;
                            break;
                        }
                    }
                    else
                    {
                        // comp
                        for(; b < encsize; ++b)
                        {
                            if(ptr[b] != byte_iterator<KEY>::const_access(target, b+byte))
                            {
                                found = false;
                                break;
                            }
                        }
                    }
                }
                offset += bytes(encsize);
            }
        } else {
            b_index = 0;
        }
        return found;
    }

    template<PTRIETPL>
    bool set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::best_match(const KEY* data, size_t length, fwdnode_t** tree_pos, base_t** node,
            uint& p_byte, uint& b_index) const {
        // run through tree as long as there are branches covering some of 
        // the encoding
        *node = fast_forward(data, length, tree_pos, p_byte);
        if((size_t)*node != (size_t)*tree_pos) {
            return bucket_search(data, length, (node_t*)*node, b_index, p_byte/2);
        } else
        {
            return false;
        }
    }


    template<PTRIETPL>
    void set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::split_fwd(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t p_byte) 
    {
        if(bsize+1 == p_byte/2)
        {
            assert(node->_count <= 256);
            return;
        }
        assert(node->_count == SPLITBOUND);

        const uint16_t bucketsize = SPLITBOUND;
        node_t lown;
        fwdnode_t* fwd_n = new fwdnode_t;

        fwd_n->_parent = jumppar;
        fwd_n->_type = 255;
        fwd_n->_path = node->_path;

        lown._path = 0;
        node->_path = masks[0];
        lown._type = 1;
        node->_type = 1;
        node->_parent = fwd_n;

        jumppar->_children[fwd_n->_path] = fwd_n;

        lown._data = nullptr;

        int lcnt = 0;
        int hcnt = 0;
        int lsize = 0;
        int hsize = 0;
        bucket_t* bucket = node->_data;
        const auto to_cut = (p_byte % 2);

        // get sizes
        uint16_t lengths[SPLITBOUND];
        for (int i = 0; i < bucketsize; ++i) {

            lengths[i] = bsize;
            if (p_byte < 4) {
                uchar* f = (uchar*)&(bucket->first(bucketsize, i));
                uchar* d = (uchar*)&(lengths[i]);
                if (p_byte > 1) {
                    lengths[i] += 1;
                    d[0] = f[1];
                    lengths[i] -= 1;
                } else {
                    d[0] = f[0];
                    d[1] = f[1];
                }
            }
            uchar b = ((uchar*)&bucket->first(bucketsize, i))[(p_byte + 1) % 2];
            b = (b >> (((p_byte)%2)*4)) & masks[0];
            if (b == 0) {
                ++lcnt;
                if (lengths[i] < (HEAPBOUND + to_cut)) {
                    if (lengths[i] > to_cut) {
                        lsize += lengths[i] - to_cut;
                    }
                    // otherwise new size = 0
                } else {
                    lsize += bytes(lengths[i]);
                }
            } else {
                ++hcnt;
                if (lengths[i] < (HEAPBOUND + to_cut)) {
                    hsize += lengths[i] - to_cut;
                } else {
                    hsize += bytes(lengths[i]);
                }
            }
        }

        // allocate new buckets
        node->_totsize = hsize > 0 ? hsize : 0;
        node->_count = hcnt;
        if (hcnt == 0) node->_data = nullptr;
        else node->_data = (bucket_t*) new uchar[node->_totsize + 
                bucket_t::overhead(node->_count)];

        lown._totsize = lsize > 0 ? lsize : 0;
        lown._count = lcnt;
        if (lcnt == 0) lown._data = nullptr;
        else lown._data = (bucket_t*) new uchar[lown._totsize +
                bucket_t::overhead(lown._count)];

        // copy values
        int lbcnt = 0;
        int hbcnt = 0;
        int bcnt = 0;

        constexpr auto move_data = [](const auto i, bucket_t* bucket, const auto bucketsize, node_t& node, const auto offset, uint16_t* lengths, int& bcnt, int& nbcnt, auto to_cut)
        {
            const auto next_length = lengths[i] - to_cut;
            const auto j = i - offset;
            node._data->first(node._count, j) = (bucket->first(bucketsize, i) << 8*to_cut);
            if (lengths[i] > 0) {
                uchar* dest = &(node._data->data(node._count)[nbcnt]);
                if (next_length >= HEAPBOUND) {
                    uchar* data = new uchar[next_length];
                    memcpy(dest, &data, sizeof (uchar*));
                    dest = data;
                }

                uchar* src;
                if (lengths[i] >= HEAPBOUND) {
                    src = *((uchar**)&(bucket->data(bucketsize)[bcnt]));
                } else {
                    src = &(bucket->data(bucketsize)[bcnt]);
                }

                if(to_cut != 0)
                {
                    if(lengths[j] > 0)
                    {
                        uchar* f = (uchar*)&(node._data->first(node._count, j));
                        f[0] = src[0];
                    }
                }

                memcpy(dest,
                        &(src[to_cut]),
                        next_length);

                if (lengths[i] >= HEAPBOUND) {
#ifndef NDEBUG
                    if (next_length >= HEAPBOUND) {
                        uchar* tmp = *((uchar**)&(node._data->data(node._count)[nbcnt]));
                        assert(tmp == dest);
                        assert(memcmp(tmp, &(src[to_cut]), next_length) == 0);
                    }
#endif
                    delete[] src;
                }
                nbcnt += bytes(next_length);
            }
            bcnt += bytes(lengths[i]);       
        };

        {   // migrate data
            auto i = 0;
            for (; i < lown._count; ++i)
                move_data(i, bucket, bucketsize, lown, 0, lengths, bcnt, lbcnt, to_cut);

            for(; i < bucketsize; ++i)
                move_data(i, bucket, bucketsize, *node, lown._count, lengths, bcnt, hbcnt, to_cut);
        }

        if (lcnt == 0) {
            if constexpr (HAS_ENTRIES)
                memcpy(node->_data->entries(bucketsize), bucket->entries(bucketsize), bucketsize * sizeof (I));
            delete[] (uchar*)bucket;
            lown._data = nullptr;
            for (size_t i = 0; i < WIDTH/2; ++i) fwd_n->_children[i] = fwd_n;
            for (size_t i = WIDTH/2; i < WIDTH; ++i) fwd_n->_children[i] = node;
            split_node(node, fwd_n, locked, bsize > 0 ? bsize - to_cut : 0, p_byte + 1);
        }
        else if (hcnt == 0) {
            if constexpr (HAS_ENTRIES)
                memcpy(lown._data->entries(bucketsize), bucket->entries(bucketsize), bucketsize * sizeof (I));
            for (size_t i = 0; i < WIDTH/2; ++i) fwd_n->_children[i] = node;
            for (size_t i = WIDTH/2; i < WIDTH; ++i) fwd_n->_children[i] = fwd_n;
            delete[] (uchar*)bucket;
            node->_data = lown._data;
            node->_path = lown._path;
            node->_count = lown._count;
            node->_totsize = lown._totsize;
            node->_type = lown._type;
            split_node(node, fwd_n, locked, bsize > 0 ? bsize - to_cut : 0, p_byte + 1);
        } else {
            node_t* low_n = new node_t;
            low_n->_data = lown._data;
            low_n->_totsize = lown._totsize;
            low_n->_count = lown._count;
            low_n->_path = lown._path;
            low_n->_type = lown._type;
            low_n->_parent =  fwd_n;
            for (size_t i = 0; i < WIDTH/2; ++i) fwd_n->_children[i] = low_n;
            for (size_t i = WIDTH/2; i < WIDTH; ++i) fwd_n->_children[i] = node;
            if constexpr (HAS_ENTRIES) {
                // We are stopping splitting here, so correct entries if needed
                I* ents = bucket->entries(bucketsize);

                for (size_t i = 0; i < bucketsize; ++i) {
                    if (i < lown._count)
                    {
                        lown._data->entries(lown._count)[i] = ents[i];
                        _entries->operator[](ents[i])._node = low_n;
                    }
                    else node->_data->entries(node->_count)[i - lown._count] = ents[i];
                }
            }
            delete[] (uchar*)bucket;
            return;
        }
    }

    template<PTRIETPL>
    void set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::split_node(node_t* node, fwdnode_t* jumppar, node_t* locked, size_t bsize, size_t p_byte) {

        assert(bsize != std::numeric_limits<size_t>::max());
        if (node->_type == 4) // new fwd node!
        {
            split_fwd(node, jumppar, locked, bsize, p_byte);
            return;
        }

        const uint16_t bucketsize = node->_count;
        const auto byte = p_byte / 2;
        
        node_t hnode;
        hnode._type = node->_type + 1;
        assert(hnode._type <= 4);
        assert(node->_type <= 4);

        assert((node->_path & masks[node->_type]) == 0);
        hnode._path = node->_path | masks[node->_type];

        // because we are only really shifting around bits when enc_pos % 8 = 0
        // then we need to find out which bit of the first 8 we are
        // splitting on in the "remainder"
        const uint r_pos = node->_type;
        assert(r_pos < 8);

        // Copy over the data to the new buckets

        int lcnt = 0;
        int hcnt = bucketsize;
        int lsize = 0;

#ifndef NDEBUG
        bool high = false;
#endif

        for (size_t i = 0; i < bucketsize; i++) {

            uchar* f = (uchar*) & node->_data->first(bucketsize, i);
            uchar fb = (f[1] >> ((p_byte+1) % 2)*4) & 0x0F; 
            if ((fb & masks[r_pos]) > 0) {
#ifndef NDEBUG
                high = true;
#else
                break;
#endif
            } else {
                assert(!high);
                ++lcnt;
                --hcnt;
                uint16_t fc;
                if (byte < 2) {
                    uchar* fcc = (uchar*) & fc;
                    if (byte == 0) {
                        fcc[0] = f[0];
                        fcc[1] = f[1];
                    } else {
                        fc = (bsize + byte);
                        fcc[0] = f[1];
                        fc -= byte;
                    }
                } else {
                    fc = bsize;
                }
                lsize += bytes(fc);
            }
        }

        bucket_t* old = node->_data;
        // copy over values
        hnode._count = hcnt;
        hnode._totsize = node->_totsize - lsize;
        node->_count = lcnt;
        node->_totsize = lsize;

        if(byte >= 2)
        {
            assert(hnode._totsize == bytes(bsize) * hnode._count);
            assert(node->_totsize == bytes(bsize) * node->_count);
        }

        size_t dist = (hnode._path - node->_path);
        assert(dist > 0);

        node->_type += 1;
        assert(node->_type <= 4);

        if (node->_count == 0) // only high node has data
        {
            for(size_t i = node->_path; i < hnode._path; ++i)
            {
                assert(jumppar->_children[i] == node);
                jumppar->_children[i] = jumppar;
            }

            node->_path = hnode._path;
            node->_count = hnode._count;
            node->_data = old;
            node->_totsize = hnode._totsize;
            node->_type = hnode._type;

            split_node(node, jumppar, locked, bsize, p_byte);
        }
        else if (hnode._count == 0) // only low node has data
        {
            for(size_t i = hnode._path; i < hnode._path + dist; ++i)
            {
                assert(jumppar->_children[i] == node);
                jumppar->_children[i] = jumppar;
            }

            node->_data = old;
            split_node(node, jumppar, locked, bsize, p_byte);
        } else {
            node_t* h_node = new node_t;
            h_node->_count = hnode._count;
            h_node->_type = hnode._type;
            h_node->_path = hnode._path;
            h_node->_totsize = hnode._totsize;
            h_node->_parent = jumppar;

            for(size_t i = hnode._path; i < hnode._path + dist; ++i)
            {
                assert(jumppar->_children[i] == node);
                jumppar->_children[i] = h_node;
            }

            h_node->_data = (bucket_t*) new uchar[h_node->_totsize + 
                    bucket_t::overhead(h_node->_count)];
            node->_data = (bucket_t*) new uchar[node->_totsize + 
                    bucket_t::overhead(node->_count)];

            // copy firsts
            memcpy(&node->_data->first(node->_count), &(old->first(bucketsize)), sizeof (uint16_t) * node->_count);
            memcpy(&h_node->_data->first(h_node->_count), &(old->first(bucketsize, node->_count)), sizeof (uint16_t) * h_node->_count);

            // copy data
            memcpy(node->_data->data(node->_count), old->data(bucketsize), node->_totsize);
            memcpy(h_node->_data->data(h_node->_count), &(old->data(bucketsize)[node->_totsize]), h_node->_totsize);

            if constexpr (HAS_ENTRIES) {
                I* ents = old->entries(bucketsize);

                for (size_t i = 0; i < bucketsize; ++i) {
                    if (i < node->_count) node->_data->entries(node->_count)[i] = ents[i];
                    else
                    {
                        h_node->_data->entries(h_node->_count)[i - node->_count] = ents[i];
                        _entries->operator[](ents[i])._node = h_node;
                    }
                }
            }

            delete[] (uchar*)old;
            if(node->_count >= SPLITBOUND && node->_count >= h_node->_count)
            {
                split_node(node, jumppar, locked, bsize, p_byte);
            }
            if(h_node->_count >= SPLITBOUND)
            {
                split_node(h_node, jumppar, nullptr, bsize, p_byte);
            }
        }
    }

    template<PTRIETPL>
    std::pair<bool, size_t>
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::exists(const KEY* data, size_t length) const {
        assert(length <= 65536);

        uint b_index = 0;

        fwdnode_t* fwd = const_cast<fwdnode_t*>(&_root);
        base_t* base = nullptr;
        uint byte = 0;

        b_index = 0;
        bool res = best_match(data, length, &fwd, &base, byte, b_index);
        returntype_t ret = returntype_t(res, std::numeric_limits<size_t>::max());
        if((size_t)fwd != (size_t)base) {
            node_t* node = (node_t*)base;
            if (HAS_ENTRIES && res) {
                ret.second = node->_data->entries(node->_count)[b_index];
            }
        }
        return ret;
    }

    template<PTRIETPL>
    returntype_t
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::insert(const KEY* data, size_t length) {
        assert(length <= 65536);
        const auto size = byte_iterator<KEY>::element_size() * length;
        uint b_index = 0;

        fwdnode_t* fwd = &_root;
        node_t* node = nullptr;
        base_t* base = nullptr;
        uint p_byte = 0;

        bool res = best_match(data, length, &fwd, &base, p_byte, b_index);
        if (res) { // We are not inserting duplicates, semantics of PTrie is a set.
            returntype_t ret(false, 0);
            if constexpr (HAS_ENTRIES) {
                node = (node_t*)base;
                ret = returntype_t(false, node->_data->entries(node->_count)[b_index]);
            }
            return ret;
        }
        const auto byte = p_byte / 2;
        if(base == (base_t*)fwd)
        {
            node = new node_t;
            node->_count = 0;
            node->_data = nullptr;
            node->_type = 0;
            node->_path = 0;
            node->_parent = fwd;
            node->_totsize = 0;
            assert(node);

            uchar* sc = (uchar*) & size;
            uchar b = (byte < 2 ? sc[1 - byte] : byte_iterator<KEY>::const_access(data, byte-2));
            b = (b >> ((p_byte+1) % 2)*4) & 0x0F;

            uchar min = b;
            uchar max = b;
            int bit = 4;
            bool stop = false;
            do {
                --bit;

                min = min & (~masks[bit]);
                max |= masks[bit];
                for (int i = min; i <= max ; ++i) {
                    if(fwd->_children[i] != fwd)
                    {
                        max = (max & ~masks[bit]) | (masks[bit] & b);
                        min = min | (masks[bit] & b);
                        bit += 1;
                        stop = true;
                        break;
                    }
                }
            } while(bit > 0 && !stop);

            for (size_t i = min; i <= max; ++i) fwd->_children[i] = node;
            node->_path = min;
            node->_type = bit;
        } else
        {
            node = (node_t*)base;
        }

        // make a new bucket, add new entry, copy over old data
        const auto nenc_size = size >= byte ? size-byte : 0;

        
        uint nbucketcount = node->_count + 1;
        uint nitemsize = nenc_size;
        bool copyval = true;
        if (nitemsize >= HEAPBOUND) {
            copyval = false;
            nitemsize = sizeof (uchar*);
        }

        uint nbucketsize = node->_totsize + nitemsize;

        bucket_t* nbucket = (bucket_t*) new uchar[nbucketsize + 
                bucket_t::overhead(nbucketcount)];

        // copy over old "firsts"
        memcpy(&nbucket->first(nbucketcount), &(node->_data->first(node->_count)), b_index * sizeof (uint16_t));
        memcpy(&(nbucket->first(nbucketcount, b_index + 1)), &(node->_data->first(node->_count, b_index)),
                (node->_count - b_index) * sizeof (uint16_t));

        uchar* f = (uchar*) & nbucket->first(nbucketcount, b_index);
        if (byte >= 2) {
            f[1] = byte_iterator<KEY>::const_access(data, -2 + byte);
            if(byte - 1 < length)
                f[0] = byte_iterator<KEY>::const_access(data, -2 + byte + 1);
            else
                f[0] = 0;
        } else {
            nbucket->first(nbucketcount, b_index) = size;
            if (byte == 1) {
                nbucket->first(nbucketcount, b_index) <<= 8;
                f[0] = byte_iterator<KEY>::const_access(data, 0);
            }
        }

        size_t entry = 0;
        if constexpr (HAS_ENTRIES) {
            // copy over entries
            memcpy(nbucket->entries(nbucketcount), node->_data->entries(node->_count), b_index * sizeof (I));
            memcpy(&(nbucket->entries(nbucketcount)[b_index + 1]), &(node->_data->entries(node->_count)[b_index]),
                    (node->_count - b_index) * sizeof (I));

            entry = nbucket->entries(nbucketcount)[b_index] = _entries->next(0);
            entry_t& ent = _entries->operator[](entry);
            ent._node = node;
        }



        uint tmpsize = 0;
        if (byte >= 2) tmpsize = b_index * bytes(nenc_size);
        else {
            uint16_t o = size;
            for (size_t i = 0; i < b_index; ++i) {

                uint16_t f = node->_data->first(nbucketcount - 1, i);
                uchar* fc = (uchar*) & f;
                uchar* oc = (uchar*) & o;
                if (byte != 0) {
                    f >>= 8;
                    fc[1] = oc[1];
                    f -= 1;
                }
                tmpsize += bytes(f);
            }
        }
        // copy over old data
        memcpy(nbucket->data(nbucketcount),
                node->_data->data(node->_count), tmpsize);

        memcpy(&(nbucket->data(nbucketcount)[tmpsize + nitemsize]),
                &(node->_data->data(node->_count)[tmpsize]), (node->_totsize - tmpsize));

        // copy over new data
        if (copyval) {
            if constexpr (byte_iterator<KEY>::continious())
                memcpy(&(nbucket->data(nbucketcount)[tmpsize]),
                        &byte_iterator<KEY>::const_access(data, byte), nenc_size);
            else
            {
                for(size_t i = 0; i < nenc_size; ++i)
                    nbucket->data(nbucketcount)[tmpsize+i] = 
                            byte_iterator<KEY>::const_access(data, byte+i);
            }
        } else {
            // alloc space
            uchar* dest = new uchar[nenc_size];

            // copy data to heap
            if constexpr (byte_iterator<KEY>::continious())
                memcpy(dest, &byte_iterator<KEY>::const_access(data, byte), nenc_size);
            else
                for(size_t i = 0; i < nenc_size; ++i)
                    dest[i] = byte_iterator<KEY>::const_access(data, byte+i);

            // copy pointer in
            memcpy(&(nbucket->data(nbucketcount)[tmpsize]),
                    &dest, sizeof (uchar*));
        }

        // if needed, split the node 
        delete[] (uchar*)node->_data;
        node->_data = nbucket;
        node->_count = nbucketcount;
        node->_totsize = nbucketsize;

        if (node->_count >= SPLITBOUND) {
            // copy over data to we can work while readers finish
            // we have to wait for readers to finish for 
            // tree extension
            split_node(node, fwd, node, nenc_size, p_byte);
        }

#ifndef NDEBUG        
        for (int i = byte - 1; i >= 2; --i) {
            assert(fwd != nullptr);
            assert(fwd->_parent == nullptr || fwd->_parent->_children[fwd->_path] == fwd);
            fwd = fwd->_parent;

        }
        auto r = exists(data, length);
        if (!r.first) {

            r = exists(data, length);
            assert(false);
        }
#endif
        return returntype_t(true, entry);
    }
    
    template<PTRIETPL>
    void
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::inject_byte(node_t* node, uchar topush, size_t totsize, std::function<uint16_t(size_t)> _sizes)
    {
        bucket_t *nbucket = node->_data;
        if(totsize > 0) {
            nbucket = (bucket_t *) new uchar[totsize +
                                          bucket_t::overhead(node->_count)];
        }

        size_t dcnt = 0;
        size_t ocnt = 0;
        for(size_t i = 0; i < node->_count; ++i)
        {
            auto const size = _sizes(i);
            uchar* f = (uchar*)&nbucket->first(node->_count, i);
            nbucket->first(node->_count, i) = node->_data->first(node->_count, i);
            uchar push = f[0];
            f[0] = f[1];
            f[1] = topush;
            // in some cases we need to expand to heap here!
            if(size > 0)
            {
                if(size < HEAPBOUND)
                {
                    nbucket->data(node->_count)[dcnt] = push;
                    dcnt += 1;
                }
                if(size < HEAPBOUND && size > 1)
                {
                    memcpy(&(nbucket->data(node->_count)[dcnt]),
                               &(node->_data->data(node->_count)[ocnt]),
                                       size - 1);
                    ocnt += size - 1;
                    dcnt += size - 1;
                }
                else if(size >= HEAPBOUND)
                {
                    uchar* src = nullptr;
                    uchar* dest = new uchar[size];
                    memcpy(&(nbucket->data(node->_count)[dcnt]), &dest, sizeof(size_t));
                    ++dest;
                    dcnt += sizeof(size_t);
                    if(size == HEAPBOUND)
                    {
                        src = &(node->_data->data(node->_count)[ocnt]);
                        memcpy(dest, src, size - 1);
                        ocnt += size - 1;
                    }
                    else
                    {
                        assert(size > HEAPBOUND);
                        // allready on heap, but we need to expand it
                        src = *(uchar**)&(node->_data->data(node->_count)[ocnt]);
                        memcpy(dest, src, size - 1);
                        ocnt += sizeof(size_t);
                    }
                    --dest;
                    dest[0] = push;
                }
            }
            // also, handle entries here!
        }

        assert(ocnt == node->_totsize);
        assert(totsize == dcnt);

        if(nbucket != node->_data) delete[] (uchar*)node->_data;

        node->_data = nbucket;
    }


    template<PTRIETPL>
    bool
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::merge_down(fwdnode_t* parent, node_t* node, int on_heap)
    {
        if(node->_type == 0)
        {
            if(node->_count < SPLITBOUND/3) return true;
            if(node->_count == 0)
            {
                for(size_t i = 0; i < WIDTH; ++i) parent->_children[i] = parent;
                delete node;
                do {
                    if (parent != &_root) {
                        // we can remove fwd and go back one level
                        parent->_parent->_children[parent->_path] = parent->_parent;
                        ++on_heap;
                        fwdnode_t* next = parent->_parent;
                        delete parent;
                        parent = next;
                        base_t* other = parent;
                        for(size_t i = 0; i < WIDTH; ++i)
                        {
                            if(parent->_children[i] != parent && other != parent->_children[i])
                            {
                                if(other != parent)
                                {
                                    other = nullptr;
                                    break;
                                }
                                else
                                {
                                    other = parent->_children[i];
                                }
                            }
                        }

                        if(other == nullptr)
                        {
                            return true;
                        }
                        else if(other->_type != 255)
                        {
                            node = (node_t*)other;
                            return merge_down(parent, node, on_heap);
                        }
                        else if(other != parent)
                        {
                            assert(other->_type == 255);
                            return true;
                        }

                    } else {
                        return true;
                    }
                } while(true);
            }
            else if(parent != &_root)
            {
                // we need to re-add path to items here.
                if(parent->_parent == &_root) {
                    // something
                    uint16_t sizes[WIDTH];
                    size_t totsize = 0;
                    for(size_t i = 0; i < node->_count; ++i)
                    {
                        uint16_t t = 0;
                        uchar* tc = (uchar*)&t;
                        uchar* fc = (uchar*)&node->_data->first(node->_count, i);
                        tc[0] = fc[1];
                        tc[1] = parent->_path;
                        sizes[i] = t;
                        totsize += bytes(sizes[i]);
                    }

                    inject_byte(node, parent->_path, totsize, [&sizes](size_t i )
                    {
                        return sizes[i];
                    });

                    node->_path = parent->_path;
                    node->_parent = parent->_parent;
                    parent->_parent->_children[node->_path] = node;
                    node->_type = 8;
                    node->_totsize = totsize;
                    fwdnode_t* next = parent->_parent;
                    delete parent;
                    return merge_down(next, node, on_heap + 1);
                }
                else
                {
                    assert(node->_count > 0);
                    if(on_heap == std::numeric_limits<int>::min())
                    {
                        int depth = 0;
                        uint16_t length = 0;
                        uchar* l = (uchar*)&length;
                        fwdnode_t* tmp = parent;

                        while(tmp != &_root)
                        {
                            l[0] = l[1];
                            l[1] = tmp->_path;
                            tmp = tmp->_parent;
                            ++depth;
                        }
                        assert(length + 1 >= depth);
                        on_heap = length;
                        on_heap -= depth;
                    }

                    on_heap += 1;
                    // first copy in path to firsts.

                    assert(on_heap >= 0);
                    node->_path = parent->_path;
                    parent->_parent->_children[node->_path] = node;
                    fwdnode_t* next = parent->_parent;
                    delete parent;
                    parent = next;
                    node->_type = 8;

                    if(on_heap == 0)
                    {
                        for(size_t i = 0; i < node->_count; ++i)
                        {
                            uchar* f = (uchar*)&node->_data->first(node->_count, i);
                            f[0] = f[1];
                            f[1] = node->_path;
                        }
                    }
                    else if(on_heap > 0)
                    {
                        size_t nbucketsize = 0;
                        if(on_heap >= HEAPBOUND)
                        {
                            nbucketsize = node->_count * sizeof(size_t);
                        }
                        else//if(on_heap < HEAPBOUND)
                        {
                            assert(on_heap >= 0);
                            nbucketsize = on_heap * node->_count;
                        }

                        inject_byte(node, node->_path, nbucketsize, [on_heap](size_t)
                        {
                            return on_heap;
                        });

                        node->_totsize = nbucketsize;
                    }
                    return merge_down(next, node, on_heap + 1);
                }
            }
            if(parent != &_root)
            {
                assert(node->_count > 0);
                return merge_down(parent->_parent, node, on_heap);
            }
        }
        else
        {
            if(node->_count > SPLITBOUND / 3) return true;
            uchar path = node->_path;
            base_t* child;
            if(path & masks[node->_type - 1])
            {
                child = parent->_children[
                         path & ~masks[node->_type - 1]];
            }
            else
            {
                child = parent->_children[
                        path | masks[node->_type - 1]];
            }

            assert(node != child);

            if(child->_type != node->_type && child->_type != 255)
            {
                // The other node is not ready for merging yet.
                assert(child->_type > node->_type);
                return false;
            }
            else
            {

                if(child->_type != 255) {
                    node_t *other = (node_t *) child;

                    const uint nbucketcount = node->_count + other->_count;
                    const uint nbucketsize = node->_totsize + other->_totsize;

                    if (nbucketcount >= SPLITBOUND)
                        return false;

                    bucket_t *nbucket = (bucket_t *) new uchar[nbucketsize +
                                                            bucket_t::overhead(nbucketcount)];
                    node_t *first = node;
                    node_t *second = other;
                    if (path & masks[node->_type - 1]) {
                        std::swap(first, second);
                    }

                    memcpy(&nbucket->first(nbucketcount),
                           &(first->_data->first(first->_count)),
                           first->_count * sizeof(uint16_t));

                    memcpy(&(nbucket->first(nbucketcount, first->_count)),
                           &(second->_data->first(second->_count)),
                           second->_count * sizeof(uint16_t));

                    if constexpr (HAS_ENTRIES) {
                        // copy over entries
                        memcpy(nbucket->entries(nbucketcount),
                               first->_data->entries(first->_count),
                               first->_count * sizeof(I));
                        memcpy(&(nbucket->entries(nbucketcount)[first->_count]),
                               second->_data->entries(second->_count),
                               second->_count * sizeof(I));

                    }

                    // copy over old data
                    if (nbucketsize > 0) {
                        memcpy(nbucket->data(nbucketcount),
                               first->_data->data(first->_count), first->_totsize);

                        memcpy(&(nbucket->data(nbucketcount)[first->_totsize]),
                               second->_data->data(second->_count), second->_totsize);

                    }
                    delete[] (uchar*)node->_data;
                    node->_data = nbucket;
                    node->_totsize = nbucketsize;
                    node->_count = nbucketcount;
                }
                uchar from = node->_path & ~masks[node->_type - 1];
                uchar to = from;
                for(size_t i = node->_type - 1; i < 8; ++i) {
                    to = to | masks[i];
                }

                if(child->_type == 255)
                {
                    if(child != parent) return true;
                    for(size_t i = from; i <= to; ++i)
                    {
                        if( parent->_children[i] != child &&
                            parent->_children[i] != node)
                            return  true;
                    }
                }

                node->_type -= 1;
                node->_path = from;

                for(size_t i = from; i <= to; ++i)
                {
                    assert(parent->_children[i] == child ||
                       parent->_children[i] == node);
                    parent->_children[i] = node;
                }
                return merge_down(parent, node, on_heap);
            }
        }
        return true;
    }

    template<PTRIETPL>
    void
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::erase(fwdnode_t* parent, node_t* node, size_t bindex, int on_heap)
    {
        
        // first find size and amount before
        uint16_t size = 0;
        uint16_t before = 0;
        if (parent == &_root)
        {
            for(size_t i = 0; i < bindex; ++i)
            {
               before += bytes(node->_data->first(node->_count, i));
            }
            size = node->_data->first(node->_count, bindex);
        }
        else if(parent->_parent == &_root) {
             for(size_t i = 0; i <= bindex; ++i)
             {
                 uint16_t t = 0;
                 uchar* tc = (uchar*)&t;
                 uchar* fc = (uchar*)&node->_data->first(node->_count, i);
                 tc[0] = fc[1];
                 tc[1] = parent->_path;
                 --t;
                 if(i == bindex) size = t;
                 else before += bytes(t);
             }
        } else {
            assert(on_heap != std::numeric_limits<int>::min());
            size = on_heap > 0 ? on_heap : 0;
            before = size * bindex;
        }

        // got sizes, now we can remove data if we point to anything
        if(size >= HEAPBOUND)
        {
            before = sizeof(size_t)*bindex;
            uchar* src = *((uchar**)&(node->_data->data(node->_count)[before]));
            delete[] src;
            size = sizeof(size_t);
        }

        uint nbucketcount = node->_count - 1;
        if(nbucketcount > 0) {
            uint nbucketsize = node->_totsize - size;

            bucket_t *nbucket = (bucket_t *) new uchar[nbucketsize +
                                                    bucket_t::overhead(nbucketcount)];

            // copy over old "firsts", [0,bindex) to [0,bindex) then (bindex,node->_count) to [bindex, nbucketcount)
            memcpy(&nbucket->first(nbucketcount),
                   &(node->_data->first(node->_count)),
                   bindex * sizeof(uint16_t));

            memcpy(&(nbucket->first(nbucketcount, bindex)),
                       &(node->_data->first(node->_count, bindex + 1)),
                       (nbucketcount - bindex) * sizeof(uint16_t));

            if constexpr (HAS_ENTRIES) {
                // copy over entries
                memcpy(nbucket->entries(nbucketcount),
                       node->_data->entries(node->_count),
                       bindex * sizeof(I));
                memcpy(&(nbucket->entries(nbucketcount)[bindex]),
                       &(node->_data->entries(node->_count)[bindex + 1]),
                       (nbucketcount - bindex) * sizeof(I));

                // copy back entries here in _entries!
                // TODO fixme!
            }

            // copy over old data
            if (nbucketsize > 0) {
                memcpy(nbucket->data(nbucketcount),
                       node->_data->data(node->_count), before);
                assert(nbucketsize >= before);
                memcpy(&(nbucket->data(nbucketcount)[before]),
                       &(node->_data->data(node->_count)[before + size]),
                       (nbucketsize - before));

            }
            delete[] (uchar*)node->_data;
            node->_data = nbucket;
            node->_count = nbucketcount;
            node->_totsize -= size;

        }
        else
        {
            delete[] (uchar*)node->_data;
            node->_data = nullptr;
            node->_count = 0;
            node->_totsize = 0;
        }

        merge_down(parent, node, on_heap);
    }

    template<PTRIETPL>
    bool
    set<KEY, HEAPBOUND, SPLITBOUND, ALLOCSIZE, T, I, HAS_ENTRIES>::erase(const KEY *data, size_t length)
    {
        const auto size = length*byte_iterator<KEY>::element_size();
        assert(size <= 65536);
        uint b_index = 0;

        fwdnode_t* fwd = &_root;
        base_t* base = nullptr;
        uint byte = 0;

        b_index = 0;
        bool res = this->best_match(data, length, &fwd, &base, byte, b_index);
        if(!res || (size_t)fwd == (size_t)base)
        {
            assert(!this->exists(data, length).first);
            return false;
        }
        else
        {
            int onheap = size;
            onheap -= byte;
            erase(fwd, (node_t *) base, b_index, onheap);
            assert(!this->exists(data, length).first);
            return true;
        }
    }
}



#endif /* PTRIE_H */
