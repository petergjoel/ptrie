/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2016  Peter Gjøl Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   ptrie_map.h
 * Author: Peter G. Jensen
 *
 *
 * Created on 06 October 2016, 13:51
 */

#ifndef PTRIE_MAP_H
#define PTRIE_MAP_H
#include "ptrie.h"


namespace ptrie {

    template<
    uint16_t HEAPBOUND = 128,
    uint16_t SPLITBOUND = 128,
    size_t ALLOCSIZE = (1024 * 64),
    size_t FWDALLOC = 256,
    typename T = void,
    typename I = size_t
    >
    class set_stable : public set<HEAPBOUND, SPLITBOUND, ALLOCSIZE, FWDALLOC, T, I> {
        using pt = set<PTRIEDEF>;
    public:
        set_stable() : pt()
        {
            this->_entries = new linked_bucket_t<typename pt::entry_t, ALLOCSIZE>(1);
        }

        size_t size() const {
            return this->_entries->size();
        }

        size_t unpack(I index, uchar* destination);

        bool         erase (binarywrapper_t wrapper);
        bool         erase (const uchar* data, size_t length);

    protected:
        void erase(typename pt::fwdnode_t* parent, typename pt::node_t* node, size_t bucketid, int byte);
        bool merge_down(typename pt::fwdnode_t* parent, typename pt::node_t* node, int byte);
        void inject_byte(typename pt::node_t* node, uchar topush, size_t totsize, auto sizes);
   };

    template<PTRIETPL>
    size_t
    set_stable<PTRIEDEF>::unpack(I index, uchar* destination) {
        typename pt::node_t* node = NULL;
        typename pt::fwdnode_t* par = NULL;
        // we can find size without bothering anyone (to much)        
        std::stack<uchar> path;
        size_t bindex = 0;
        {
#ifndef NDEBUG
            bool found = false;
#endif
            typename pt::entry_t& ent = this->_entries->operator[](index);
            par = (typename pt::fwdnode_t*)ent.node;
            node = (typename pt::node_t*)par->_children[ent.path];
            typename pt::bucket_t* bckt = node->_data;
            I* ents = bckt->entries(node->_count, true);
            for (size_t i = 0; i < node->_count; ++i) {
                if (ents[i] == index) {
                    bindex = i;
#ifndef NDEBUG
                    found = true;
#endif
                    break;
                }
            }
            assert(found);
        }



        while (par != this->_root) {
            path.push(par->_path);
            par = par->_parent;
        }

        uint16_t size = 0;
        size_t offset = 0;
        size_t ps = path.size();
        if (ps <= 1) {
            size = node->_data->first(0, bindex);
            if (ps == 1) {
                size >>= 8;
                uchar* bs = (uchar*) & size;
                bs[1] = path.top();
                path.pop();
            }

            uint16_t o = size;
            for (size_t i = 0; i < bindex; ++i) {

                uint16_t f = node->_data->first(0, i);
                uchar* fc = (uchar*) & f;
                uchar* oc = (uchar*) & o;
                if (ps != 0) {
                    f >>= 8;
                    fc[1] = oc[1];
                    f -= 1;
                }
                offset += pt::bytes(f);
                //                assert(bytes(f) == nbucket->bytes(i));
            }
        } else {
            uchar* bs = (uchar*) & size;
            bs[1] = path.top();
            path.pop();
            bs[0] = path.top();
            path.pop();
            offset = (pt::bytes(size - ps) * bindex);
        }


        if (size > ps) {
            uchar* src;
            if ((size - ps) >= HEAPBOUND) {
                src = *((uchar**)&(node->_data->data(node->_count, true)[offset]));
            } else {
                src = &(node->_data->data(node->_count, true)[offset]);
            }

            memcpy(&(destination[ps]), src, (size - ps));
        }

        uint16_t first = node->_data->first(0, bindex);

        size_t pos = 0;
        while (!path.empty()) {
            destination[pos] = path.top();
            path.pop();
            ++pos;
        }


        if (ps > 0) {
            uchar* fc = (uchar*) & first;
            if (ps > 1) {
                destination[pos] = fc[1];
                ++pos;
            }
            destination[pos] = fc[0];
            ++pos;
        }
        
        return size;
    }

    template<PTRIETPL>
    void
    set_stable<PTRIEDEF>::inject_byte(typename pt::node_t* node, uchar topush, size_t totsize, auto _sizes)
    {
        const bool hasent = true;
        typename pt::bucket_t *nbucket = node->_data;
        if(totsize > 0) {
            nbucket = (typename pt::bucket_t *) malloc(totsize +
                                          pt::bucket_t::overhead(node->_count,
                                                             hasent));
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
                    nbucket->data(node->_count, hasent)[dcnt] = push;
                    dcnt += 1;
                }
                if(size < HEAPBOUND && size > 1)
                {
                    memcpy(&(nbucket->data(node->_count, hasent)[dcnt]),
                               &(node->_data->data(node->_count, hasent)[ocnt]),
                                       size - 1);
                    ocnt += size - 1;
                    dcnt += size - 1;
                }
                else if(size >= HEAPBOUND)
                {
                    uchar* src = NULL;
                    uchar* dest = (uchar*)malloc(size);
                    memcpy(&(nbucket->data(node->_count, hasent)[dcnt]), &dest, sizeof(size_t));
                    ++dest;
                    dcnt += sizeof(size_t);
                    if(size == HEAPBOUND)
                    {
                        src = &(node->_data->data(node->_count, hasent)[ocnt]);
                        memcpy(dest, src, size - 1);
                        ocnt += size - 1;
                    }
                    else
                    {
                        assert(size > HEAPBOUND);
                        // allready on heap, but we need to expand it
                        src = *(uchar**)&(node->_data->data(node->_count, hasent)[ocnt]);
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

        if(nbucket != node->_data) free(node->_data);

        node->_data = nbucket;
    }


    template<PTRIETPL>
    bool
    set_stable<PTRIEDEF>::merge_down(typename pt::fwdnode_t* parent, typename pt::node_t* node, int on_heap)
    {
        const bool hasent = true;
        if(node->_type == 0)
        {
            if(node->_count == 0)
            {
                do {
                    if (parent != this->_root) {
                        // we can remove fwd and go back one level
                        parent->_parent->_children[parent->_path] = parent->_parent;
                        ++on_heap;
                        parent = parent->_parent;
                        typename pt::base_t* other = parent;
                        for(size_t i = 0; i < 256; ++i)
                        {
                            if(parent->_children[i] != parent && other != parent->_children[i])
                            {
                                if(other != parent)
                                {
                                    other = NULL;
                                    break;
                                }
                                else
                                {
                                    other = parent->_children[i];
                                }
                            }
                        }

                        if(other == NULL)
                        {
                            return true;
                        }
                        else if(other->_type != 255)
                        {
                            node = (typename pt::node_t*)other;
                            if(node->_count <= SPLITBOUND / 3) {
                                return merge_down(parent, node, on_heap);
                            }
                            else
                            {
                                return true;
                            }
                        }
                        else if(other != parent)
                        {
                            assert(other->_type == 255);
                            return true;
                        }

                    } else {
                        // clear everything!
                        this->init();
                        return true;
                    }
                } while(true);
            }
            else if(parent != this->_root)
            {
                // we need to re-add path to items here.
                if(parent->_parent == this->_root) {
                    // something
                    uint16_t sizes[256];
                    size_t totsize = 0;
                    for(size_t i = 0; i < node->_count; ++i)
                    {
                        uint16_t t = 0;
                        uchar* tc = (uchar*)&t;
                        uchar* fc = (uchar*)&node->_data->first(node->_count, i);
                        tc[0] = fc[1];
                        tc[1] = parent->_path;
                        sizes[i] = t;
                        totsize += pt::bytes(sizes[i]);
                    }

                    inject_byte(node, parent->_path, totsize, [&sizes](size_t i )
                    {
                        return sizes[i];
                    });

                    node->_path = parent->_path;
                    parent->_parent->_children[node->_path] = node;
                    node->_type = 8;
                    node->_totsize = totsize;
                    return merge_down(parent->_parent, node, on_heap + 1);
                }
                else
                {
                    assert(node->_count > 0);
                    if(on_heap == std::numeric_limits<int>::min())
                    {
                        int depth = 0;
                        uint16_t length = 0;
                        uchar* l = (uchar*)&length;
                        typename pt::fwdnode_t* tmp = parent;

                        while(tmp != this->_root)
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
                    node->_type = 8;

                    if(on_heap == 0)
                    {
                        for(size_t i = 0; i < node->_count; ++i)
                        {
                            uchar* f = (uchar*)&node->_data->first(node->_count, i);
                            f[0] = f[1];
                            f[1] = parent->_path;
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

                        inject_byte(node, parent->_path, nbucketsize, [on_heap](size_t)
                        {
                            return on_heap;
                        });

                        node->_totsize = nbucketsize;
                    }
                }
            }
            if(node->_count <= SPLITBOUND / 3 && parent != this->_root)
            {
                assert(node->_count > 0);
                return merge_down(parent->_parent, node, on_heap);
            }
        }
        else
        {
            uchar path = node->_path;
            typename pt::base_t* child;
            if(path & binarywrapper_t::_masks[node->_type - 1])
            {
                child = parent->_children[
                         path & ~binarywrapper_t::_masks[node->_type - 1]];
            }
            else
            {
                child = parent->_children[
                        path | binarywrapper_t::_masks[node->_type - 1]];
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
                    typename pt::node_t *other = (typename pt::node_t *) child;

                    const uint nbucketcount = node->_count + other->_count;
                    const uint nbucketsize = node->_totsize + other->_totsize;

                    if (nbucketcount >= SPLITBOUND)
                        return false;

                    typename pt::bucket_t *nbucket = (typename pt::bucket_t *) malloc(nbucketsize +
                                                            pt::bucket_t::overhead(nbucketcount, hasent));
                    typename pt::node_t *first = node;
                    typename pt::node_t *second = other;
                    if (path & binarywrapper_t::_masks[node->_type - 1]) {
                        std::swap(first, second);
                    }

                    memcpy(&nbucket->first(nbucketcount),
                           &(first->_data->first(first->_count)),
                           first->_count * sizeof(uint16_t));

                    memcpy(&(nbucket->first(nbucketcount, first->_count)),
                           &(second->_data->first(second->_count)),
                           second->_count * sizeof(uint16_t));

                    if (hasent) {
                        // copy over entries
                        memcpy(nbucket->entries(nbucketcount, true),
                               first->_data->entries(first->_count, true),
                               first->_count * sizeof(I));
                        memcpy(&(nbucket->entries(nbucketcount, true)[first->_count]),
                               second->_data->entries(second->_count, true),
                               second->_count * sizeof(I));

                    }

                    // copy over old data
                    if (nbucketsize > 0) {
                        memcpy(nbucket->data(nbucketcount, hasent),
                               first->_data->data(first->_count, hasent), first->_totsize);

                        memcpy(&(nbucket->data(nbucketcount, hasent)[first->_totsize]),
                               second->_data->data(second->_count, hasent), second->_totsize);

                    }
                    free(node->_data);
                    node->_data = nbucket;
                    node->_totsize = nbucketsize;
                    node->_count = nbucketcount;
                }
                uchar from = node->_path & ~binarywrapper_t::_masks[node->_type - 1];
                uchar to = from;
                for(size_t i = node->_type - 1; i < 8; ++i) {
                    to = to | binarywrapper_t::_masks[i];
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
                if(node->_count <= SPLITBOUND / 3)
                {
                    assert(node->_count > 0);
                    return merge_down(parent, node, on_heap);
                }
                else
                {
                    return true;
                }
            }
        }
        return true;
    }

    template<PTRIETPL>
    void
    set_stable<PTRIEDEF>::erase(typename pt::fwdnode_t* parent, typename pt::node_t* node, size_t bindex, int on_heap)
    {
        const bool hasent = true;

        // first find size and amount before
        uint16_t size = 0;
        uint16_t before = 0;
        if (parent == this->_root)
        {
            for(size_t i = 0; i < bindex; ++i)
            {
               before += pt::bytes(node->_data->first(node->_count, i));
            }
            size = node->_data->first(node->_count, bindex);
        }
        else if(parent->_parent == this->_root) {
             for(size_t i = 0; i <= bindex; ++i)
             {
                 uint16_t t = 0;
                 uchar* tc = (uchar*)&t;
                 uchar* fc = (uchar*)&node->_data->first(node->_count, i);
                 tc[0] = fc[1];
                 tc[1] = parent->_path;
                 --t;
                 if(i == bindex) size = t;
                 else before += pt::bytes(t);
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
            uchar* src = *((uchar**)&(node->_data->data(node->_count, hasent)[before]));
            free(src);
            size = sizeof(size_t);
        }

        uint nbucketcount = node->_count - 1;
        if(nbucketcount > 0) {
            uint nbucketsize = node->_totsize - size;

            typename pt::bucket_t *nbucket = (typename pt::bucket_t *) malloc(nbucketsize +
                                                    pt::bucket_t::overhead(nbucketcount, hasent));

            // copy over old "firsts", [0,bindex) to [0,bindex) then (bindex,node->_count) to [bindex, nbucketcount)
            memcpy(&nbucket->first(nbucketcount),
                   &(node->_data->first(node->_count)),
                   bindex * sizeof(uint16_t));

            memcpy(&(nbucket->first(nbucketcount, bindex)),
                       &(node->_data->first(node->_count, bindex + 1)),
                       (nbucketcount - bindex) * sizeof(uint16_t));

            size_t entry = 0;
            if (hasent) {
                // copy over entries
                memcpy(nbucket->entries(nbucketcount, true),
                       node->_data->entries(node->_count, true),
                       bindex * sizeof(I));
                memcpy(&(nbucket->entries(nbucketcount, true)[bindex]),
                       &(node->_data->entries(node->_count, true)[bindex + 1]),
                       (nbucketcount - bindex) * sizeof(I));

                // copy back entries here in _entries!
                // TODO fixme!
            }

            // copy over old data
            if (size > 0) {
                memcpy(nbucket->data(nbucketcount, hasent),
                       node->_data->data(node->_count, hasent), before);
                assert(nbucketsize >= before);
                memcpy(&(nbucket->data(nbucketcount, hasent)[before]),
                       &(node->_data->data(node->_count, hasent)[before + size]),
                       (nbucketsize - before));

            }
            free(node->_data);
            node->_data = nbucket;
        }
        else
        {
            free(node->_data);
            node->_data = NULL;
        }
        node->_count = nbucketcount;
        node->_totsize -= size;
        if(nbucketcount <= SPLITBOUND / 3)
        {
            merge_down(parent, node, on_heap);
        }
    }

    template<PTRIETPL>
    bool
    set_stable<PTRIEDEF>::erase(binarywrapper_t encoding)
    {
        uint b_index = 0;

        typename pt::fwdnode_t* fwd = this->_root;
        typename pt::base_t* base = NULL;
        uint byte = 0;

        b_index = 0;
        bool res = this->best_match(encoding, &fwd, &base, byte, b_index);
        returntype_t ret = returntype_t(res, std::numeric_limits<size_t>::max());
        if(!res || (size_t)fwd == (size_t)base)
        {
            return false;
        }
        else
        {
            int onheap = encoding.size();
            onheap -= byte;
            erase(fwd, (typename pt::node_t *) base, b_index, onheap);
            assert(!this->exists(encoding).first);
            return true;
        }
    }

    template<PTRIETPL>
    bool
    set_stable<PTRIEDEF>::erase(const uchar *data, size_t length)
    {
        binarywrapper_t b(data, length*8);
        return erase(b);
    }

}


#endif /* PTRIE_MAP_H */