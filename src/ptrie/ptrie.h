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
#include <cstring>
#include <functional>
#include <memory>
#include <tuple>

#include "linked_bucket.h"



// direct 2lvl indexing in chunks of ~ 2^32
// takes up ~ 512k (256k on 32bit) for the index.
// when to keep data in bucket or send to heap - just make sure bucket does not overflow

namespace ptrie {
    typedef uint32_t uint;
    typedef unsigned char uchar;
    
    template<typename D, typename N>
    struct __ptrie_el_t {
        N _node;
        D _data;
    };

    template<typename N>
    struct __ptrie_el_t<void, N> {
        N _node;
    };
    typedef std::pair<bool, size_t> returntype_t;

    struct __base_t {
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

    // not sure if we can inherit from std::iterator here, its ill-defined
    // what e.g. a "distance".
    template<typename N, size_t BDIV, size_t BSIZE, size_t HEAPBOUND>
    void __build_path(const N* node, std::stack<uchar>& path, uint16_t bindex, size_t& offset, size_t& ps, uint16_t& size);

    template<typename N, typename KEY, size_t BDIV, size_t BSIZE, size_t HEAPBOUND>
    void __write_data(KEY* dest, const N* node, std::stack<uchar>& path, size_t bindex, size_t offset, size_t ps, uint16_t size);
    
    
    constexpr uint16_t __memsize(uint16_t d, size_t HEAPBOUND) {
        if (d >= HEAPBOUND) return sizeof (uchar*);
        else return d;
    }

    template<typename P, typename R>
    class __iterator {
    protected:
        const __base_t* _node = nullptr;
        int16_t _index = 0;
        template<int16_t INC, int16_t MAX>
        bool move()
        {
            if(_node->_type == 255)
            {
                auto* fwd = static_cast<const typename P::fwdnode_t*>(_node);
                while(fwd->_children[_index] == fwd) 
                    _index += INC;
                if(_index == MAX+INC) // we have reached the end of this fwdnode
                {
                    if(fwd->_parent == nullptr)
                        return false; // we have reached the end of structure
                    int i = MAX;
                    while(fwd->_parent->_children[i] != fwd)
                        i -= INC; // find next element in parent (or end of parent)

                    _node = fwd->_parent;
                    _index = i+INC;
                    return true;
                }
                else
                {
                    _node = fwd->_children[_index];
                    _index = 255-MAX;
                    if(_node->_type != 255)
                    {
                        if constexpr (MAX == 0) _index = static_cast<const typename P::node_t*>(_node)->_count-1;
                        return false;
                    }
                    else
                        return true;
                }
            }
            else
            {
                _index += INC;
                auto* node = static_cast<const typename P::node_t*>(_node);
                if(MAX == 255 && _index < node->_count)
                    return false;
                else if(MAX == 0 && _index >= 0)
                    return false;
                int i = MAX;
                while(node->_parent->_children[i] != node)
                    i -= INC; // find next element in parent (or end of parent)
                _node = node->_parent;
                _index = i+INC;
                return true;
            }                
        }
    public:
        __iterator(const __base_t* base, int16_t index)
        : _node(base), _index(index) {}
        __iterator() = default;
        __iterator(const __iterator&) = default;
        __iterator(__iterator&&) = default;
        __iterator& operator=(const __iterator&) = default;
        __iterator& operator=(__iterator&&) = default;

        bool operator==(const __iterator& other) const {
            if(_node->_type != 255)
                return _node == other._node && _index == other._index; 
            if(_index <= 0 && other._index <= 0) return true; // begin;
            else if(_index >= 255 && other._index >= 255) return true; // end
            return false;
        }
        bool operator!=(const __iterator& other) const { return !(*this == other); }

        R& operator++()
        {
            if(_node->_type == 255)
                _index = std::max<int16_t>(0, _index);
            if(move<1,255>())
                ++(*static_cast<R*>(this));
            return *static_cast<R*>(this);
        }

        R operator++(int)
        {
            auto cpy = *static_cast<R*>(this);
            ++(*static_cast<R*>(this));
            return cpy;
        }

        R& operator--()
        {
            if(_node->_type == 255)
                _index = std::min<int16_t>(255, _index);
            if(move<-1,0>())
                --(*static_cast<R*>(this));
            return *static_cast<R*>(this);
        }

        R operator--(int)
        {
            auto cpy = *static_cast<R*>(this);
            --(*static_cast<R*>(this));
            return cpy;
        }

        size_t
        unpack(typename P::key_t* dest) const {
            size_t ps, offset;
            uint16_t size;
            std::stack<uchar> path;
            auto node = static_cast<const typename P::node_t*>(_node);
            __build_path<typename P::node_t, P::bdiv, P::bsize, P::heapbound>
                (node, path, _index, offset, ps, size);
            __write_data<typename P::node_t, typename P::key_t, P::bdiv, P::bsize, P::heapbound>
                (dest, node, path, _index, offset, ps, size);
            return size/byte_iterator<typename P::key_t>::element_size();
        }

        std::vector<typename P::key_t>
        unpack() const {
            size_t ps, offset;
            uint16_t size;
            std::stack<uchar> path;
            auto node = static_cast<const typename P::node_t*>(_node);
            __build_path<typename P::node_t, P::bdiv, P::bsize, P::heapbound>
                (node, path, _index, offset, ps, size);
            std::vector<typename P::key_t> destination(size/byte_iterator<typename P::key_t>::element_size());
            __write_data<typename P::node_t, typename P::key_t, P::bdiv, P::bsize, P::heapbound>
                (destination.data(), node, path, _index, offset, ps, size);        
            return destination;   
        }

        void
        unpack(std::vector<typename P::key_t>& dest) const {
            size_t ps, offset;
            uint16_t size;
            std::stack<uchar> path;
            auto node = static_cast<const typename P::node_t*>(_node);
            __build_path<typename P::node_t, P::bdiv, P::bsize, P::heapbound>
                (node, path, _index, offset, ps, size);
            dest.resize(size/byte_iterator<typename P::key_t>::element_size());
            __write_data<typename P::node_t, typename P::key_t, P::bdiv, P::bsize, P::heapbound>
                (node, path, _index, offset, ps, size);
        }                
    };
    
#define PTRIETPL typename KEY, uint16_t HEAPBOUND, uint16_t SPLITBOUND, uint8_t BSIZE, size_t ALLOCSIZE, typename T, typename I, bool HAS_ENTRIES
#define PTRIETLPA KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, T, I, HAS_ENTRIES
    
    template<
    typename KEY = uchar,
    uint16_t HEAPBOUND = 17,
    uint16_t SPLITBOUND = 129,
    uint8_t BSIZE = 8,
    size_t ALLOCSIZE = (1024 * 64),
    typename T = void,
    typename I = size_t,
    bool HAS_ENTRIES = false
    >
    class __ptrie {
    public:
        struct fwdnode_t;
        struct node_t;
    protected:
        static_assert(BSIZE == 2 || BSIZE == 4 || BSIZE == 8);

        static constexpr auto WIDTH = 1 << BSIZE;
        static constexpr auto BDIV = 8/BSIZE;
        static constexpr auto FILTER = 0xFF >> (8-BSIZE);

        static_assert(HEAPBOUND * SPLITBOUND < std::numeric_limits<uint16_t>::max(),
                "HEAPBOUND * SPLITBOUND should be less than 2^16");

        static_assert(SPLITBOUND >= 6,
                "SPLITBOUND MUST BE LARGER THAN 6");

        static_assert(HEAPBOUND > sizeof(size_t),
                "HEAPBOUND MUST BE LARGER THAN sizeof(size_t)");
                
    protected:

        typedef __ptrie_el_t<T, node_t*> entry_t;
        using entrylist_t = linked_bucket_t<entry_t, ALLOCSIZE>;
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
    public:
        struct node_t : public __base_t {
            uint16_t _count = 0; // bucket-counts
            uint32_t _totsize = 0; 
            fwdnode_t* _parent = nullptr;
            bucket_t* _data = nullptr; // back-pointers to data-array up to date
            void cleanup(size_t depth, uint16_t remainder);
            constexpr uchar* data() const { return _data->data(_count); }
            constexpr uint16_t& first(size_t index) const { return _data->first(_count, index); }
            constexpr uint16_t* first() const { return &_data->first(_count, 0); }
            constexpr I* entries() const { return _data->entries(_count); }
            constexpr void clone(const node_t& other, entrylist_t* entries, const entrylist_t* other_entries, uint16_t esize, size_t depth);
        };

        struct fwdnode_t : public __base_t {
            __base_t* _children[WIDTH];
            fwdnode_t* _parent;
            constexpr void clone(const fwdnode_t& other, entrylist_t* entries, const entrylist_t* other_entries, uint16_t esize, size_t depth);
            size_t dist_to(fwdnode_t* other) const
            {
                assert(this);
                if(this == other) return 0;
                assert(_parent != nullptr);
                return 1 + _parent->dist_to(other);
            }
            
            constexpr uchar get_byte() const {
                return _get_byte(BDIV);
            }
            
            constexpr uchar _get_byte(size_t i) const
            {
                if(i == 1) return _path;
                return _path | (_parent->_get_byte(i-1) << BSIZE);
            }
        };
    protected:
        std::shared_ptr<entrylist_t> _entries = nullptr;

        fwdnode_t _root;

        __base_t* fast_forward(const KEY* data, size_t length, fwdnode_t** tree_pos, uint& byte) const;
        bool bucket_search(const KEY* data, size_t length, node_t* node, uint& b_index, uint byte) const;

        bool best_match(const KEY* data, size_t length, fwdnode_t** tree_pos, __base_t** node, uint& byte, uint& b_index) const;

        void split_node(node_t* node, fwdnode_t* jumppar, node_t* locked, int32_t bsize, size_t byte);

        void split_fwd(node_t* node, fwdnode_t* jumppar, node_t* locked, int32_t bsize, size_t byte);

        static constexpr uint16_t bytes(const uint16_t d) {
            return __memsize(d, HEAPBOUND);
        }

        void init();

        void erase(node_t* node, size_t bucketid, int on_heap, const KEY* data, size_t byte);
        // helper-functions for erase
        void merge_down(node_t* node, int on_heap, const KEY* data, size_t byte);
        void merge_regular(node_t* node, int on_heap, const KEY* data, size_t byte);
        bool merge_nodes(node_t* node, node_t* other, uchar path);
        void merge_empty(node_t* node, int on_heap, const KEY* data, size_t byte);
        void readd_sizes(node_t* node, fwdnode_t* parent, int on_heap, const KEY* data, size_t byte);
        void readd_byte(node_t* node, int on_heap, const KEY* data, size_t byte);

        void inject_byte(node_t* node, uchar topush, size_t totsize, std::function<uint16_t(size_t)> sizes);
        
        static constexpr uchar _all_masks[8] = {
            static_cast <uchar>(0x80),
            static_cast <uchar>(0x40),
            static_cast <uchar>(0x20),
            static_cast <uchar>(0x10),
            static_cast <uchar>(0x08),
            static_cast <uchar>(0x04),
            static_cast <uchar>(0x02),
            static_cast <uchar>(0x01)
        };
        static constexpr const uchar* _masks = &_all_masks[8-BSIZE];

    public:
        void move(__ptrie& other);                
    public:
        __ptrie();
        ~__ptrie();
        
        using key_t = KEY;
        static constexpr auto bsize = BSIZE;
        static constexpr auto bdiv = BDIV;
        static constexpr auto heapbound = HEAPBOUND;
        
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
        
        
        __ptrie(__ptrie&& other) { move(other); }
        
        __ptrie& operator=(__ptrie&& other) { move(other); return *this; }
        
        __ptrie(const __ptrie& other) : __ptrie()
        {
            *this = other;
        }
        __ptrie& operator=(const __ptrie& other);
        
    };

    template<
    typename KEY = uchar,
    uint16_t HEAPBOUND = 17,
    uint16_t SPLITBOUND = 129,
    uint8_t BSIZE = 8,
    size_t ALLOCSIZE = (1024 * 64)
    >
    class set : private __ptrie<KEY,HEAPBOUND,SPLITBOUND,BSIZE,ALLOCSIZE,void,size_t,false> {
        using pt = __ptrie<KEY,HEAPBOUND,SPLITBOUND,BSIZE,ALLOCSIZE,void,size_t,false>;
    public:
        using pt::__ptrie;
        using pt::insert;
        using pt::exists;
        using pt::erase;
        
        using node_t = typename pt::node_t;
        using fwdnode_t = typename pt::fwdnode_t;
        using pt::key_t; 

        static constexpr auto bsize = pt::bsize;
        static constexpr auto bdiv = pt::bdiv;
        static constexpr auto heapbound = HEAPBOUND;
        
        class iterator : public __iterator<set, iterator>
        {
        public:
            using __iterator<set, iterator>::__iterator;
        };
        
        iterator begin() const { return ++iterator(&this->_root, 0); }
        iterator end()   const { return iterator(&this->_root, 256); }
    };
    
    template<PTRIETPL>
    __ptrie<PTRIETLPA>::~__ptrie() {
        std::stack<std::tuple<fwdnode_t*,size_t, uint16_t>> stack;
        stack.emplace(&_root,0,0);
        while(!stack.empty())
        {
            auto next = stack.top();
            stack.pop();
            for(size_t i = 0; i < WIDTH; ++i)
            {
                fwdnode_t* parent = std::get<0>(next);
                __base_t* child = parent->_children[i];
                if(child != parent && child != nullptr)
                {
                    if(i > 0 && child == parent->_children[i-1]) continue;
                    if(child->_type == 255)
                    {
                        auto f = std::get<2>(next);
                        if(std::get<1>(next) / BDIV < 2)
                        {
                            // we add bits from the most significant to the least
                            f |= (child->_path << ((16-BSIZE)-(BSIZE*std::get<1>(next))));
                        }
                        assert(child->_path == i);
                        stack.emplace((fwdnode_t*)child, std::get<1>(next) + 1, f);
                    }
                    else
                    {
                        node_t* node = (node_t*) child;
                        node->cleanup(std::get<1>(next), std::get<2>(next));
                        delete node;
                    }
                }
            }
            if(&_root != std::get<0>(next))
                delete std::get<0>(next);
        }
        _entries = nullptr;
    }

    template<PTRIETPL>
    void __ptrie<PTRIETLPA>::node_t::cleanup(size_t depth, uint16_t encsize) {
        const auto bdepth = depth / BDIV;
        assert(bdepth < 2 || encsize > 0);
        if (bdepth >= 2 && 
               (encsize < bdepth || // already fully encoded
                encsize - bdepth < HEAPBOUND)) // residue is directly encoded
        {
            // nothing on heap
        } else if (bdepth >= 2) {

            // everything is allocated on heap
            auto ptr = (uchar **) data();
            for (size_t i = 0; i < _count; ++i) {
                delete[] ptr[i];
            }
        } else {
            assert(_data || _count == 0);
            size_t offset = 0;
            for (size_t i = 0; i < _count; ++i) {
                auto lencsize = encsize;
                auto f = first(i);
                if(bdepth != 0)
                {
                   // only keep most sig. bits
                   lencsize &= ~0xFF00;
                   //extract most sig. bits and shift to least sig. bits
                   lencsize |= (f & 0xFF00) >> 8;
                }
                if ((lencsize-bdepth) >= HEAPBOUND) {
                    auto ptr = (uchar **) (&(data()[offset]));
                    delete[] *ptr;
                }
                offset += bytes(lencsize-bdepth);
            }
        }
        delete[] reinterpret_cast<uchar *>(_data);
        _data = nullptr;
        _count = 0;
    }

    template<PTRIETPL>
    void __ptrie<PTRIETLPA>::init()
    {
        _entries = nullptr;
        if constexpr (HAS_ENTRIES)
        {
            _entries = std::make_unique<entrylist_t>(1);
        }

        _root._parent = nullptr;
        _root._type = 255;
        _root._path = 0;

        size_t i = 0;
        for (; i < WIDTH; ++i) _root._children[i] = &_root;
    }

    template<PTRIETPL>
    void __ptrie<PTRIETLPA>::move(__ptrie& other)
    {
        _entries = std::move(other._entries);
        _root._parent = nullptr;
        _root._type = 255;
        _root._path = 0;
        for(size_t i = 0; i < WIDTH; ++i)
        {
            auto e = _root._children[i] = other._root._children[i];
            if(e == nullptr)
                continue;
            else if(e == &other._root)
                _root._children[i] = &_root;
            else
            {
                if(e->_type == 255)
                    static_cast<fwdnode_t*>(e)->_parent = &_root;
                else
                    static_cast<node_t*>(e)->_parent = &_root;
            }
            other._root._children[i] = nullptr;
        }
    }
    
    template<PTRIETPL>
    __ptrie<PTRIETLPA>::__ptrie()
    {
        init();
    }

    template<PTRIETPL>
    __ptrie<PTRIETLPA>& __ptrie<PTRIETLPA>::operator=(const ptrie::__ptrie<PTRIETLPA> &other) 
    {
        _root.clone(other._root, _entries.get(), other._entries.get(), 0, 0);
        return *this;
    }
    
    template<PTRIETPL>
    constexpr void __ptrie<PTRIETLPA>::fwdnode_t::clone(const __ptrie<PTRIETLPA>::fwdnode_t& other, entrylist_t* entries, const entrylist_t* other_entries, uint16_t esize, size_t depth)
    {
        _path = other._path;
        _type = 255;

        for(size_t i = 0; i < WIDTH; ++i)
        {
            auto* child = other._children[i];
            if(child == nullptr)
            {
                _children[i] = nullptr;
                continue;
            }
            if(&other == child)
            {
                _children[i] = this;
                continue;
            }
            if(i > 0 && child == other._children[i-1])
            {
                _children[i] = _children[i-1];
                continue;
            }
            if(child->_type == 255)
            {
                auto nn = new fwdnode_t;
                _children[i] = nn;
                auto f = esize;
                if(depth / BDIV < 2)
                {
                    // we add bits from the most significant to the least
                    f |= (child->_path << ((16-BSIZE)-(BSIZE*depth)));
                }
                nn->clone(*static_cast<const fwdnode_t*>(child), entries, other_entries, f, depth+1);
                nn->_parent = this;
            }
            else
            {
                auto nn = new node_t;
                _children[i] = nn;
                nn->clone(*static_cast<const node_t*>(child), entries, other_entries, esize, depth);
                nn->_parent = this;
            }
        }
    }

    template<PTRIETPL>
    constexpr void __ptrie<PTRIETLPA>::node_t::clone(const __ptrie<PTRIETLPA>::node_t& other, entrylist_t* entries, const entrylist_t* other_entries, uint16_t encsize, size_t depth) {
        const auto bdepth = depth / BDIV;
        _path = other._path;
        _type = other._type;
        _count = other._count;
        _totsize = other._totsize;
        _data = (bucket_t*) new uchar[_totsize + bucket_t::overhead(_count)];
        std::copy(other.first(), other.first() + _count, first());
        if (bdepth >= 2 &&
                (encsize < bdepth || // already fully encoded
                encsize - bdepth < HEAPBOUND)) // residue is directly encoded
        {
            std::copy(other.data(), other.data() + other._totsize, data());
        } else if (bdepth >= 2) {

            // everything is allocated on heap
            auto ptr = (uchar **) data();
            auto optr = (uchar **) other.data();
            for (size_t i = 0; i < _count; ++i) {
                ptr[i] = new uchar[(encsize - bdepth)];
                std::copy(optr[i], optr[i] + encsize, ptr[i]);
            }
        } else {
            size_t offset = 0;
            for (size_t i = 0; i < _count; ++i) {
                auto lencsize = encsize;
                auto f = first(i);
                if (bdepth != 0) {
                    // only keep most sig. bits
                    lencsize &= ~0xFF00;
                    //extract most sig. bits and shift to least sig. bits
                    lencsize |= (f & 0xFF00) >> 8;
                }
                if ((lencsize - bdepth) >= HEAPBOUND) {
                    auto ptr = (uchar **) (&(data()[offset]));
                    auto optr = (uchar **) (&(other.data()[offset]));
                    ptr[0] = new uchar[lencsize];
                    std::copy(optr[0], optr[0] + lencsize, ptr[0]);
                } else {
                    std::copy(other.data() + offset, other.data() + lencsize, data() + offset);
                }
                offset += bytes(lencsize - bdepth);
            }
        }
        if constexpr (HAS_ENTRIES) {
            for (size_t i = 0; i < _count; ++i) {
                auto eid = entries->next(0);
                this->entries()[i] = eid;
                (*entries)[eid] = (*other_entries)[other.entries()[i]];
            }
        }
    }

    template<PTRIETPL>
    __base_t*
    __ptrie<PTRIETLPA>::fast_forward(const KEY* data, size_t s, fwdnode_t** tree_pos, uint& p_byte) const {
        fwdnode_t* t_pos = *tree_pos;

        uchar* sc = (uchar*) & s;
        
        do {
            *tree_pos = t_pos;

            __base_t* next;
            const auto byte = p_byte / BDIV;
            uchar nb;
            if (byte >= 2) nb = byte_iterator<KEY>::const_access(data, byte - 2);
            else nb = sc[1 - byte];
            if constexpr (BSIZE != 8)
                nb = (nb >> (((BDIV - 1) - (p_byte % BDIV))*BSIZE)) & FILTER;
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
    bool __ptrie<PTRIETLPA>::bucket_search(const KEY* target, size_t size, node_t* node, uint& b_index, uint byte) const {
        // run through the stored data in the bucket, looking for matches
        // start by creating an encoding that "points" to the "unmatched"
        // part of the encoding. Notice, this is a shallow copy, no actual
        // heap-allocation happens!
        bool found = false;
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
            if(byte - 1 < size)
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
                uchar ob;

                if (bucket->first(node->_count, b_index) > first) break;
                // first is 2 bytes, which is size of counter, the first part of the tree
                // if we reach here, things have same size

                if (encsize < HEAPBOUND) {
                    for (; b < encsize; ++b) {
                        ob = byte_iterator<KEY>::const_access(target, b+byte);
                        if (data[offset + b] != ob) break;
                    }
                    if (b == encsize) {
                        found = true;
                        break;
                    } else {
                        assert(ob != data[offset + b]);
                        if (ob < data[offset + b]) {
                            found = false;
                            break;
                        }
                        // else continue search
                    }
                } else {
                    uchar* ptr = *((uchar**) (&data[offset]));
                    if constexpr(byte_iterator<KEY>::continious())
                    {
                        int cmp = std::memcmp(ptr, &byte_iterator<KEY>::const_access(target, byte), encsize);
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
                            ob = byte_iterator<KEY>::const_access(target, b+byte);
                            if(ptr[b] != ob)
                                break;
                        }
                        if (b == encsize)
                        {
                            found = true;
                            break;
                        } else {
                            assert(ob != ptr[b]);
                            if (ob < ptr[b]) {
                                found = false;
                                break;
                            }
                            // else continue search                            
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
    bool __ptrie<PTRIETLPA>::best_match(const KEY* data, size_t length, fwdnode_t** tree_pos, __base_t** node,
            uint& p_byte, uint& b_index) const {
        // run through tree as long as there are branches covering some of 
        // the encoding
        *node = fast_forward(data, length, tree_pos, p_byte);
        if((__base_t*)*node != (__base_t*)*tree_pos) {
            return bucket_search(data, length, (node_t*)*node, b_index, p_byte/BDIV);
        } 
        else
        {
            return false;
        }
    }


    template<PTRIETPL>
    void __ptrie<PTRIETLPA>::split_fwd(node_t* node, fwdnode_t* jumppar, node_t* locked, int32_t bsize, size_t p_byte) 
    {
        if(bsize == -1)
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
        assert(fwd_n->_path < WIDTH);
        
        lown._path = 0;
        node->_path = _masks[0];
        assert(node->_path < WIDTH);
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
        int to_cut;
        if constexpr (BSIZE != 8)
            to_cut = ((p_byte + 1) % BDIV)== 0 ? 1 : 0;
        else
            to_cut = 1;

        // get sizes
        uint16_t lengths[SPLITBOUND];
        for (int i = 0; i < bucketsize; ++i) {

            lengths[i] = std::max(bsize, 0);
            if (p_byte < BDIV*2) {
                uchar* f = (uchar*)&(bucket->first(bucketsize, i));
                uchar* d = (uchar*)&(lengths[i]);
                if (p_byte >= BDIV) {
                    lengths[i] += 1;
                    d[0] = f[1];
                    lengths[i] -= 1;
                } else {
                    d[0] = f[0];
                    d[1] = f[1];
                }
            }
            uchar b = ((uchar*)&bucket->first(bucketsize, i))[1-to_cut];
            if constexpr (BSIZE != 8)
                b = (b >> (((BDIV-1)-((p_byte + 1) % BDIV))*BSIZE));
            b &= _masks[0];
            if (b == 0) {
                ++lcnt;
                assert(hcnt == 0);
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
        else if(to_cut != 0 || lcnt != 0) 
            node->_data = (bucket_t*) new uchar[node->_totsize + 
                bucket_t::overhead(node->_count)];

        lown._totsize = lsize > 0 ? lsize : 0;
        lown._count = lcnt;
        if (lcnt == 0) lown._data = nullptr;
        else if(to_cut != 0 || hcnt != 0) 
            lown._data = (bucket_t*) new uchar[lown._totsize +
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
                uchar* dest = &(node.data()[nbcnt]);
                if (next_length >= HEAPBOUND) {
                    uchar* data = new uchar[next_length];
                    *reinterpret_cast<uchar**>(dest) = data;
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

                std::copy(src + to_cut, src + to_cut + next_length, dest);

                if (lengths[i] >= HEAPBOUND) {
#ifndef NDEBUG
                    if (next_length >= HEAPBOUND) {
                        uchar* tmp = *((uchar**)&(node.data()[nbcnt]));
                        assert(tmp == dest);
                        assert(std::memcmp(tmp, &(src[to_cut]), next_length) == 0);
                    }
#endif
                    delete[] src;
                }
                nbcnt += bytes(next_length);
            }
            bcnt += bytes(lengths[i]);       
        };

        if(std::min(lcnt, hcnt) != 0 || to_cut != 0) {   // migrate data
            auto i = 0;
            for (; i < lown._count; ++i)
                move_data(i, bucket, bucketsize, lown, 0, lengths, bcnt, lbcnt, to_cut);

            for(; i < bucketsize; ++i)
                move_data(i, bucket, bucketsize, *node, lown._count, lengths, bcnt, hbcnt, to_cut);
        }

        if (lcnt == 0) {
            if(to_cut != 0)
            {
                if constexpr (HAS_ENTRIES)
                {
                    auto* e = bucket->entries(bucketsize);
                    std::copy(e, e + bucketsize, node->_data->entries(bucketsize));
                }
                delete[] (uchar*)bucket;
                lown._data = nullptr;
            } else node->_data = bucket;
            
            for (size_t i = 0; i < WIDTH/2; ++i) fwd_n->_children[i] = fwd_n;
            for (size_t i = WIDTH/2; i < WIDTH; ++i) fwd_n->_children[i] = node;
            
            split_node(node, fwd_n, locked, bsize - to_cut, p_byte + 1);
        }
        else if (hcnt == 0) {
            if(to_cut != 0)
            {
                if constexpr (HAS_ENTRIES)
                {
                    auto* e = bucket->entries(bucketsize);
                    std::copy(e, e+ bucketsize, lown._data->entries(bucketsize));
                }
                delete[] (uchar*)bucket;
                node->_data = lown._data;
            } else node->_data = bucket;
            
            for (size_t i = 0; i < WIDTH/2; ++i) fwd_n->_children[i] = node;
            for (size_t i = WIDTH/2; i < WIDTH; ++i) fwd_n->_children[i] = fwd_n;
            
            node->_path = lown._path;
            assert(node->_path < WIDTH);
            node->_count = lown._count;
            node->_totsize = lown._totsize;
            node->_type = lown._type;
            split_node(node, fwd_n, locked, bsize - to_cut, p_byte + 1);
        } else {
            node_t* low_n = new node_t;
            low_n->_data = lown._data;
            low_n->_totsize = lown._totsize;
            low_n->_count = lown._count;
            low_n->_path = lown._path;
            assert(low_n->_path < WIDTH);
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
        }
    }

    template<PTRIETPL>
    void __ptrie<PTRIETLPA>::split_node(node_t* node, fwdnode_t* jumppar, node_t* locked, int32_t bsize, size_t p_byte) {

        assert(bsize >= -1);
        assert(bsize < std::numeric_limits<int32_t>::max());
        if (node->_type == BSIZE) // new fwd node!
        {
            split_fwd(node, jumppar, locked, bsize, p_byte);
            return;
        }

        const uint16_t bucketsize = node->_count;
        const auto byte = p_byte / BDIV;
        
        node_t hnode;
        hnode._type = node->_type + 1;
        assert(hnode._type <= BSIZE);
        assert(node->_type <= BSIZE);

        assert((node->_path & _masks[node->_type]) == 0);
        hnode._path = node->_path | _masks[node->_type];
        assert(hnode._path < WIDTH);
            
        // because we are only really shifting around bits when enc_pos % 8 = 0
        // then we need to find out which bit of the first 8 we are
        // splitting on in the "remainder"
        const uint r_pos = node->_type;
        assert(r_pos <= BSIZE);

        // Copy over the data to the new buckets

        int lcnt = 0;
        int hcnt = bucketsize;
        int lsize = 0;

#ifndef NDEBUG
        bool high = false;
#endif

        for (size_t i = 0; i < bucketsize; i++) {

            auto f = (uchar*)&node->_data->first(bucketsize, i);
            uchar fb = (f[1] >> (((BDIV - 1) - (p_byte % BDIV)) * BSIZE)); 
            if ((fb & _masks[r_pos]) > 0) {
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
                        fc = (std::max(bsize,0) + byte);
                        fcc[0] = f[1];
                        fc -= byte;
                    }
                } else {
                    fc = std::max(bsize,0);
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
            assert(hnode._totsize == bytes(std::max(bsize,0)) * hnode._count);
            assert(node->_totsize == bytes(std::max(bsize,0)) * node->_count);
        }

        size_t dist = (hnode._path - node->_path);
        assert(dist > 0);

        node->_type += 1;
        assert(node->_type <= BSIZE);

        if (node->_count == 0) // only high node has data
        {
            for(size_t i = node->_path; i < hnode._path; ++i)
            {
                assert(jumppar->_children[i] == node);
                jumppar->_children[i] = jumppar;
            }

            node->_path = hnode._path;
            assert(node->_path < WIDTH);
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
            assert(h_node->_path < WIDTH);
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
            {
                uint16_t* src = &(old->first(bucketsize));
                uint16_t* mid = src + node->_count;
                uint16_t* end = mid + h_node->_count;
                std::copy(src, mid, &node->_data->first(node->_count));
                std::copy(mid, end, &h_node->_data->first(h_node->_count));
            }

            // copy data
            {
                uchar* src = old->data(bucketsize);
                uchar* mid = src + node->_totsize;
                uchar* end = mid + h_node->_totsize;
                std::copy(src, mid, node->data());
                std::copy(mid, end, h_node->data());
            }

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
            assert(node->_count < SPLITBOUND || (std::max(bsize,0)+1 == (int64_t)p_byte/BDIV));
            assert(h_node->_count < SPLITBOUND || (std::max(bsize,0)+1 == (int64_t)p_byte/BDIV));
        }
    }

    template<PTRIETPL>
    std::pair<bool, size_t>
    __ptrie<PTRIETLPA>::exists(const KEY* data, size_t length) const {
        assert(length <= 65536);

        uint b_index = 0;

        fwdnode_t* fwd = const_cast<fwdnode_t*>(&_root);
        __base_t* base = nullptr;
        uint byte = 0;

        b_index = 0;
        bool res = best_match(data, length*byte_iterator<KEY>::element_size(), &fwd, &base, byte, b_index);
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
    __ptrie<PTRIETLPA>::insert(const KEY* data, size_t length) {
        assert(length <= 65536);
        const auto size = byte_iterator<KEY>::element_size() * length;
        uint b_index = 0;

        fwdnode_t* fwd = &_root;
        node_t* node = nullptr;
        __base_t* base = nullptr;
        uint p_byte = 0;

        bool res = best_match(data, size, &fwd, &base, p_byte, b_index);
        if (res) { // We are not inserting duplicates, semantics of PTrie is a set.
            returntype_t ret(false, 0);
            if constexpr (HAS_ENTRIES) {
                node = (node_t*)base;
                ret = returntype_t(false, node->_data->entries(node->_count)[b_index]);
            }
            return ret;
        }
        const auto byte = p_byte / BDIV;
        if(base == (__base_t*)fwd)
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
            if constexpr (BSIZE != 8)
                b = (b >> (((BDIV - 1) - (p_byte % BDIV))*BSIZE)) & FILTER;

            uchar min = b;
            uchar max = b;
            int bit = BSIZE;
            bool stop = false;
            do {
                --bit;

                min = min & (~_masks[bit]);
                max |= _masks[bit];
                for (int i = min; i <= max ; ++i) {
                    if(fwd->_children[i] != fwd)
                    {
                        max = (max & ~_masks[bit]) | (_masks[bit] & b);
                        min = min | (_masks[bit] & b);
                        bit += 1;
                        stop = true;
                        break;
                    }
                }
            } while(bit > 0 && !stop);

            for (size_t i = min; i <= max; ++i) fwd->_children[i] = node;
            node->_path = min;
            assert(node->_path < WIDTH);
            node->_type = bit;
        } else
        {
            node = (node_t*)base;
        }

        // make a new bucket, add new entry, copy over old data
        const int32_t nenc_size = ((int32_t)size)-byte;

        
        uint nbucketcount = node->_count + 1;
        uint nitemsize = std::max(nenc_size, 0);
        bool copyval = true;
        if (nitemsize >= HEAPBOUND) {
            copyval = false;
            nitemsize = sizeof (uchar*);
        }

        uint nbucketsize = node->_totsize + nitemsize;

        bucket_t* nbucket = (bucket_t*) new uchar[nbucketsize + 
                bucket_t::overhead(nbucketcount)];

        // copy over old "firsts"

        {
            auto* src = &(node->_data->first(node->_count));
            auto* mid = src + b_index;
            auto* end = mid + (node->_count - b_index);
            std::copy(src, mid, &nbucket->first(nbucketcount));
            std::copy(mid, end, &nbucket->first(nbucketcount, b_index + 1));
        }

        uchar* f = (uchar*) & nbucket->first(nbucketcount, b_index);
        if (byte >= 2) {
            f[1] = byte_iterator<KEY>::const_access(data, -2 + byte);
            if(byte - 1 < size)
                f[0] = byte_iterator<KEY>::const_access(data, -2 + byte + 1);
            else
            {
                f[0] = 0;
            }
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
            {
                auto* src =  node->_data->entries(node->_count);
                auto* mid = src + b_index;
                auto* end = mid + (node->_count - b_index);
                std::copy(src, mid, nbucket->entries(nbucketcount));
                std::copy(mid, end, nbucket->entries(nbucketcount) + b_index + 1);
            }

            entry = nbucket->entries(nbucketcount)[b_index] = _entries->next(0);
            entry_t& ent = _entries->operator[](entry);
            ent._node = node;
        }



        uint tmpsize = 0;
        if (byte >= 2) tmpsize = b_index * bytes(std::max(nenc_size, 0));
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
        {
            auto* src = node->data();
            auto* mid = src + tmpsize;
            auto* end = mid + (node->_totsize - tmpsize);
            std::copy(src, mid, nbucket->data(nbucketcount));
            std::copy(mid, end, nbucket->data(nbucketcount) + tmpsize + nitemsize);
        }

        // copy over new data
        if (copyval) {
            if constexpr (byte_iterator<KEY>::continious())
            {
                auto* src = &byte_iterator<KEY>::const_access(data, byte);
                auto* end = src + std::max(nenc_size, 0);
                std::copy(src, end, nbucket->data(nbucketcount) + tmpsize);
            }
            else
            {
                for(auto i = 0; i < nenc_size; ++i)
                    nbucket->data(nbucketcount)[tmpsize+i] = 
                            byte_iterator<KEY>::const_access(data, byte+i);
            }
        } else {
            // alloc space
            uchar* dest = new uchar[std::max(nenc_size, 0)];

            // copy data to heap
            if constexpr (byte_iterator<KEY>::continious())
            {
                auto* dp = &byte_iterator<KEY>::const_access(data, byte);
                std::copy(dp, dp + std::max(nenc_size, 0), dest);
            }
            else
                for(auto i = 0; i < nenc_size; ++i)
                    dest[i] = byte_iterator<KEY>::const_access(data, byte+i);

            // copy pointer in
            *reinterpret_cast<uchar**>(nbucket->data(nbucketcount) + tmpsize) = dest;
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
    __ptrie<PTRIETLPA>::inject_byte(node_t* node, uchar topush, size_t totsize, std::function<uint16_t(size_t)> _sizes)
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
                    std::copy(node->data() + ocnt, node->data() + ocnt + size - 1, 
                            nbucket->data(node->_count) + dcnt);
                    ocnt += size - 1;
                    dcnt += size - 1;
                }
                else if(size >= HEAPBOUND)
                {
                    uchar* src = nullptr;
                    uchar* dest = new uchar[size];
                    *reinterpret_cast<uchar**>(nbucket->data(node->_count) + dcnt) = dest;
                    ++dest;
                    dcnt += sizeof(size_t);
                    if(size == HEAPBOUND)
                    {
                        src = node->data() + ocnt;
                        std::copy(src, src + (size - 1), dest);
                        ocnt += size - 1;
                    }
                    else
                    {
                        assert(size > HEAPBOUND);
                        // allready on heap, but we need to expand it
                        src = *reinterpret_cast<uchar**>(node->data() + ocnt);
                        std::copy(src, src + (size-1), dest);
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
    void 
    __ptrie<PTRIETLPA>::merge_empty(node_t* node, int on_heap, const KEY* data, size_t byte)
    {
        /*
         * If a node is emtpy, we can remove all the way down til we meet some 
         * non-empty node
         */
        assert(node->_count == 0);
        auto parent = node->_parent;
        for(size_t i = 0; i < WIDTH; ++i) parent->_children[i] = parent;
        delete node;
        do {
            if (parent != &_root) {
                // we can remove fwd and go back one level
                parent->_parent->_children[parent->_path] = parent->_parent;
                --byte;
                if((byte % BDIV) == 0)
                    ++on_heap;
                fwdnode_t* next = parent->_parent;
                delete parent;
                parent = next;
                __base_t* other = parent;
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
                    return;
                }
                else if(other->_type != 255)
                {
                    node = (node_t*)other;
                    return merge_down(node, on_heap, data, byte);
                }
                else if(other != parent)
                {
                    assert(other->_type == 255);
                    return;
                }

            } else {
                return;
            }
        } while(true);        
    }
    
    template<PTRIETPL>
    bool 
    __ptrie<PTRIETLPA>::merge_nodes(node_t* node, node_t* other, uchar path)
    {
        /*
         * Migrate data from "other"-node to "node"
         */
        assert(node->_count > 0);
        assert(other->_count > 0);
        const uint nbucketcount = node->_count + other->_count;
        const uint nbucketsize = node->_totsize + other->_totsize;

        if (nbucketcount >= SPLITBOUND)
            return false;

        bucket_t *nbucket = (bucket_t *) new uchar[nbucketsize +
                                                bucket_t::overhead(nbucketcount)];
        node_t *first = node;
        node_t *second = other;
        if (path & _masks[node->_type - 1]) {
            std::swap(first, second);
        }

        std::copy(first->first(), first->first() + first->_count,
            &nbucket->first(nbucketcount));
        std::copy(second->first(), second->first() + second->_count,
            &nbucket->first(nbucketcount, first->_count));

        if constexpr (HAS_ENTRIES) {
            // copy over entries
            std::copy(first->entries(), first->entries() + first->_count, 
                    nbucket->entries(nbucketcount));
            std::copy(second->entries(), second->entries() + second->_count, 
                    nbucket->entries(nbucketcount) + first->_count);
        }

        // copy over old data
        if (nbucketsize > 0) {
            std::copy(first->data(), first->data() + first->_totsize, 
                    nbucket->data(nbucketcount));
            std::copy(second->data(), second->data() + second->_totsize, 
                    nbucket->data(nbucketcount) + first->_totsize);
        }
        delete[] (uchar*)node->_data;
        node->_data = nbucket;
        node->_totsize = nbucketsize;
        node->_count = nbucketcount;
        return true;
    }

    template<PTRIETPL>
    void
    __ptrie<PTRIETLPA>::readd_sizes(node_t* node, fwdnode_t* parent, int on_heap, const KEY* data, size_t byte)
    {
        assert(node);
        assert(byte > 0);
        /*
         * we are re-adding one of the size-bytes. I.e. we are soon at the root
         */
        assert(node->_count > 0);
        uint16_t sizes[256];
        size_t totsize = 0;
        for(size_t i = 0; i < node->_count; ++i)
        {
            uint16_t t = (byte / BDIV) + (on_heap-1);
            uchar* tc = (uchar*)&t;
            uchar* fc = (uchar*)&node->_data->first(node->_count, i);
            tc[0] = fc[1];
            sizes[i] = t;
            totsize += bytes(sizes[i]);
        }
        auto inject = parent->get_byte();
        inject_byte(node, inject, totsize, [&sizes](size_t i )
        {
            return sizes[i];
        });
        node->_totsize = totsize;
    }

    template<PTRIETPL>
    void 
    __ptrie<PTRIETLPA>::readd_byte(node_t* node, int on_heap, const KEY* data, size_t byte)
    {
        /*
         * We are removing a parent, so we need to re-add the byte from the path
         * here.
         */
        assert(node->_count > 0);
        assert(node->_parent != &_root);
        auto parent = node->_parent;
        node->_path = parent->_path;
        assert(node->_path < WIDTH);
        node->_parent = parent->_parent;
        node->_type = BSIZE;
        parent->_parent->_children[node->_path] = node;

        if((byte % BDIV) == 0)
        {
            on_heap += 1;
            if(byte <= BDIV)
            {
                readd_sizes(node, parent, on_heap, data, byte);
            }
            else
            {
                // first copy in path to firsts.

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

                uchar inject = parent->get_byte();
                inject_byte(node, inject, nbucketsize, [on_heap](size_t)
                {
                    return on_heap;
                });

                node->_totsize = nbucketsize;
            }
        }
        delete parent;

        merge_down(node, on_heap, data, byte - 1);
    }
    
    template<PTRIETPL>
    void
    __ptrie<PTRIETLPA>::merge_regular(node_t* node, int on_heap, const KEY* data, size_t byte)
    {
#ifndef NDEBUG
        {
            std::stack<fwdnode_t*> waiting;
            waiting.push(&_root);
            while(!waiting.empty())
            {
                auto n = waiting.top();
                waiting.pop();
                for(size_t i = 0; i < WIDTH; ++i)
                {
                    if(n->_children[i]->_type == 255 && n->_children[i] != n)
                    {
                        waiting.push((fwdnode_t*)n->_children[i]);
                    }

                    assert(n->_children[i] == n ||
                           n->_children[n->_children[i]->_path] == n->_children[i]);
                }
            }
        }
#endif
        /*
         * We merge a regular node
         */
        assert(node->_type > 0);
        assert(node->_type <= BSIZE);
        if(node->_count > SPLITBOUND / 3) return;
        uchar path = node->_path;
        __base_t* child;
        auto parent = node->_parent;
        if(path & _masks[node->_type - 1])
        {
            child = parent->_children[
                     path & ~_masks[node->_type - 1]];
        }
        else
        {
            child = parent->_children[
                    path | _masks[node->_type - 1]];
        }

        assert(node != child);

        if(child->_type != node->_type && child->_type != 255)
        {
            // The other node is not ready for merging yet.
            auto other = (node_t*)child;
            assert(child->_type > node->_type);
            if(node->_count == 0)
            {
                uchar from = node->_path;
                uchar to = from;
                for(size_t i = node->_type; i < BSIZE; ++i) {
                    to = to | _masks[i];
                }
                for(size_t i = from; i <= to; ++i)
                {
                    assert(node->_parent->_children[i] == node);
                    node->_parent->_children[i] = node->_parent;
                }
                delete node;
            }
            else if(other->_count <= SPLITBOUND / 3)
            {
                merge_down(other, on_heap, data, byte);
            }
            return;
        }
        else
        {

            if(child->_type != 255) {
                node_t *other = (node_t *) child;
                if(node->_count == 0)
                {
                    if(child->_type == node->_type)
                    {
                        for(auto& c : parent->_children)
                            if(c == node)
                                c = child;
                        child->_type -= 1;
                        child->_path &= ~_masks[node->_type - 1];
                        assert(child->_path < WIDTH);
                        delete node;
                        merge_down((node_t*)child, on_heap, data, byte);
                        return;
                    }
                    else
                    {
                        for(auto& c : parent->_children)
                            if(c == node)
                                c = parent;
                        delete node;
                        return;
                    }
                }
                else
                {
                    if(!merge_nodes(node, other, path))
                        return;
                }
            } 
            else if(node->_count == 0) // && childe->_type == 255
            {
                for(auto& c : parent->_children)
                    if(c == node)
                        c = parent;
                delete node;
                return;
            }
            uchar from = node->_path & ~_masks[node->_type - 1];
            uchar to = from;
            for(size_t i = node->_type - 1; i < BSIZE; ++i) {
                to = to | _masks[i];
            }

            if(child->_type == 255)
            {
                if(child != node->_parent) return;
                for(size_t i = from; i <= to; ++i)
                {
                    if( parent->_children[i] != child &&
                        parent->_children[i] != node)
                    {
                        return;
                    }
                }
            }

            node->_type -= 1;
            node->_path = from;
            assert(node->_path < WIDTH);
            
            for(size_t i = from; i <= to; ++i)
            {
                assert(parent->_children[i] == child ||
                   parent->_children[i] == node);
                parent->_children[i] = node;
            }
            merge_down(node, on_heap, data, byte);
        }
    }
    
    template<PTRIETPL>
    void
    __ptrie<PTRIETLPA>::merge_down(node_t* node, int on_heap, const KEY* data, size_t byte)
    {
        if(node->_count > SPLITBOUND/3) return;

        if(node->_type == 0)
        {
            if(node->_count == 0)
            {
                merge_empty(node, on_heap, data, byte);
            }
            else if(node->_parent != &_root)
            {
                // we need to re-add path to items here.
                readd_byte(node, on_heap, data, byte);
            }
            if(node->_parent != &_root)
            {
                assert(node->_count > 0);
                merge_down(node, on_heap, data, byte);
            }
        }
        else
        {
            merge_regular(node, on_heap, data, byte);
        }
    }

    template<PTRIETPL>
    void
    __ptrie<PTRIETLPA>::erase(node_t* node, size_t bindex, int on_heap, const KEY* data, size_t byte)
    {
        
        // first find size and amount before
        uint16_t size = 0;
        uint16_t before = 0;
        auto parent = node->_parent;
        auto dist = parent->dist_to(&_root);
        if (dist < BDIV)
        {
            for(size_t i = 0; i < bindex; ++i)
            {
               before += bytes(node->_data->first(node->_count, i));
            }
            size = node->_data->first(node->_count, bindex);
        }
        else if(dist < 2*BDIV) {
            for(size_t i = 0; i <= bindex; ++i)
            {
                uint16_t t = on_heap+1;
                uchar* tc = (uchar*)&t;
                uchar* fc = (uchar*)&node->_data->first(node->_count, i);
                tc[0] = fc[1];
                --t;
                if(i == bindex) size = t;
                else before += bytes(t);
            }
        } else {
            assert(on_heap != std::numeric_limits<int>::min());
            size = on_heap > 0 ? on_heap : 0;
            before = bytes(size) * bindex;
        }

        // got sizes, now we can remove data if we point to anything
        if(size >= HEAPBOUND)
        {
            assert(before + sizeof(size_t) <= node->_totsize);
            uchar* src = *((uchar**)&(node->data()[before]));
            delete[] src;
            size = sizeof(size_t);
        }

        uint nbucketcount = node->_count - 1;
        if(nbucketcount > 0) {
            uint nbucketsize = node->_totsize - size;

            bucket_t *nbucket = (bucket_t *) new uchar[nbucketsize +
                                                    bucket_t::overhead(nbucketcount)];

            // copy over old "firsts", [0,bindex) to [0,bindex) then (bindex,node->_count) to [bindex, nbucketcount)
            {
                auto* src = node->first();
                auto* mid = src + bindex;
                auto* end = src + node->_count; 
                auto* dest = &nbucket->first(nbucketcount); 
                std::copy(node->first(), mid, dest);
                std::copy(mid + 1, end, dest + bindex);
            }

            if constexpr (HAS_ENTRIES) {
                // copy over entries
                {
                    auto* src = node->entries();
                    auto* mid = src + bindex;
                    auto* end = src + node->_count;
                    auto* dest = node->_data->entries(node->_count);
                    std::copy(src, mid, dest);
                    std::copy(mid + 1, end, dest + bindex);
                }

                // copy back entries here in _entries!
                // TODO fixme!
            }

            // copy over old data
            if (nbucketsize > 0) {
                auto* begin = node->data();
                auto* mid = begin + before;
                auto* end = node->data() + node->_totsize;
                auto* dest = nbucket->data(nbucketcount);
                std::copy(begin, mid, dest);
                std::copy(mid + size, end, dest + before);
                assert(nbucketsize >= before);
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

        merge_down(node, on_heap, data, byte);
#ifndef NDEBUG
        {
            std::stack<fwdnode_t*> waiting;
            waiting.push(&_root);
            while(!waiting.empty())
            {
                auto n = waiting.top();
                waiting.pop();
                for(size_t i = 0; i < WIDTH; ++i)
                {
                    if(n->_children[i]->_type == 255 && n->_children[i] != n)
                    {
                        waiting.push((fwdnode_t*)n->_children[i]);
                    }
                    else if(n->_children[i]->_type <= BSIZE)
                    {
                        assert(((node_t*)n->_children[i])->_count != 0);
                    }
                    assert(n->_children[i] == n ||
                           n->_children[n->_children[i]->_path] == n->_children[i]);
                }
            }
        }
#endif
    }

    template<PTRIETPL>
    bool
    __ptrie<PTRIETLPA>::erase(const KEY *data, size_t length)
    {
        const auto size = length*byte_iterator<KEY>::element_size();
        assert(size <= 65536);
        uint b_index = 0;

        fwdnode_t* fwd = &_root;
        __base_t* base = nullptr;
        uint p_byte = 0;

        b_index = 0;
        bool res = best_match(data, size, &fwd, &base, p_byte, b_index);
        if(!res || (size_t)fwd == (size_t)base)
        {
            assert(!exists(data, length).first);
            return false;
        }
        else
        {
            int onheap = size;
            onheap -= p_byte/BDIV;

            erase((node_t *) base, b_index, onheap, data, p_byte);
            assert(!exists(data, length).first);

            return true;
        }
    }
    
    template<typename N, size_t BDIV, size_t BSIZE, size_t HEAPBOUND>
    void __build_path(const N* node, std::stack<uchar>& path, uint16_t bindex, size_t& offset, size_t& ps, uint16_t& size)
    {
        auto par = node->_parent;
        while (par && par->_parent != nullptr) {
            path.push(par->_path);
            par = par->_parent;
        }
        
        size = 0;
        offset = 0;
        ps = path.size()/BDIV;
        if (ps <= 1) {
            size = node->_data->first(0, bindex);
            if (ps == 1) {
                size >>= 8;
                uchar* bs = (uchar*) & size;
                for(size_t i = 0; i < BDIV; ++i)
                {
                    if constexpr (BSIZE < 8)
                        bs[1] <<= BSIZE;
                    bs[1] |= path.top();
                    path.pop();
                }
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
                offset += __memsize(f, HEAPBOUND);
            }
        } else {
            for(size_t i = 0; i < BDIV*2; ++i)
            {
                size <<= BSIZE;
                size |= path.top();
                path.pop();
            }
            offset = (__memsize(size - ps, HEAPBOUND) * bindex);
        }        
    }
    
    template<typename N, typename KEY, size_t BDIV, size_t BSIZE, size_t HEAPBOUND>
    static void __write_data(KEY* dest, const N* node, std::stack<uchar>& path, size_t bindex, size_t offset, size_t ps, uint16_t size)
    {
        if (size > ps) {
            uchar* src;
            if ((size - ps) >= HEAPBOUND) {
                src = *((uchar**)&(node->_data->data(node->_count)[offset]));
            } else {
                src = &(node->_data->data(node->_count)[offset]);
            }

            if constexpr (byte_iterator<KEY>::continious())
                std::copy(src, src + (size - ps), &byte_iterator<KEY>::access(dest, ps));
            else
                for(size_t i = 0; i < (size - ps); ++i)
                    byte_iterator<KEY>::access(dest, ps + i) = src[i];
        }

        uint16_t first = node->_data->first(0, bindex);

        size_t pos = 0;
        while (path.size() >= BDIV) {
            uchar b = 0;
            for(size_t i = 0; i < BDIV; ++i)
            {
                if constexpr (BSIZE < 8)
                    b <<= BSIZE;
                b |= path.top();
                path.pop();
            }
            byte_iterator<KEY>::access(dest, pos) = b;
            ++pos;
        }


        if (ps > 0) {
            uchar* fc = (uchar*) & first;
            if (ps > 1) {
                byte_iterator<KEY>::access(dest, pos) = fc[1];
                ++pos;
            }
            byte_iterator<KEY>::access(dest, pos) = fc[0];
            ++pos;
        }        
    }
}

#endif /* PTRIE_H */
