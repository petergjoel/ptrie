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

    #define SPTRIETPL typename KEY, uint16_t HEAPBOUND, uint16_t SPLITBOUND, uint8_t BSIZE, size_t ALLOCSIZE, typename T, typename I
    #define SPTRIETPLA KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, T, I
    template<
    typename KEY = unsigned char,
    uint16_t HEAPBOUND = 17,
    uint16_t SPLITBOUND = 128,
    uint8_t BSIZE = 8,
    size_t ALLOCSIZE = (1024 * 64),
    typename T = void,
    typename I = size_t
    >
    class __set_stable : protected __ptrie<KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, T, I, true> {
        using pt = __ptrie<KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, T, I, true>;
        static_assert(std::is_integral<I>::value, "I (index-type) must be an integral");
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
        
        size_t size() const {
            return this->_entries->size();
        }

        size_t unpack(I index, KEY* destination) const;
        std::vector<KEY> unpack(I index) const;
        void unpack(I index, std::vector<KEY>& destination) const;        
        
        class siterator : public __iterator<__set_stable, siterator>
        {
        public:
            siterator(const __base_t* base, int16_t index)
            : __iterator<__set_stable, siterator>(base, index) {}
            I index() const {
                return static_cast<const typename __set_stable::node_t*>(this->_node)
                        ->entries()[this->_index];
            }            
        };
        
        friend class siterator;
    protected:
        node_t* find_metadata(I index, size_t& bindex) const;
        using entrylist_t = typename pt::entrylist_t;
  };

    template<SPTRIETPL>
    typename __set_stable<SPTRIETPLA>::node_t* 
    __set_stable<SPTRIETPLA>::
    find_metadata(I index, size_t& bindex) const
    {
        node_t* node = nullptr;
        // we can find size without bothering anyone (to much)        
        
        bindex = 0;
        {
#ifndef NDEBUG
            bool found = false;
#endif
            typename pt::entry_t& ent = this->_entries->operator[](index);
            node = ent._node;
            typename pt::bucket_t* bckt = node->_data;
            I* ents = bckt->entries(node->_count);
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
        return node;
    }       
  
    template<SPTRIETPL>
    size_t
    __set_stable<SPTRIETPLA>::unpack(I index, KEY* dest) const {
        size_t bindex;
        auto node = find_metadata(index, bindex);
        return siterator(node, bindex).unpack(dest);
    }
    
    template<SPTRIETPL>
    std::vector<KEY>
    __set_stable<SPTRIETPLA>::unpack(I index) const {
        size_t bindex;
        auto node = find_metadata(index, bindex);
        return siterator(node, bindex).unpack();
    }

    template<SPTRIETPL>
    void
    __set_stable<SPTRIETPLA>::unpack(I index, std::vector<KEY>& dest) const {
        size_t bindex;
        auto node = find_metadata(index, bindex);
        return siterator(node, bindex).unpack(dest);
    }
    
    template<
    typename KEY = unsigned char,
    typename I = size_t,
    uint16_t HEAPBOUND = 17,
    uint16_t SPLITBOUND = 128,
    uint8_t BSIZE = 8,
    size_t ALLOCSIZE = (1024 * 64)
    >
    class set_stable : private __set_stable<KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, void, I>
    {
        using pt = __set_stable<KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, void, I>;
        using iterator = typename pt::siterator;
        public:
            using pt::__ptrie;
            using pt::insert;
            using pt::exists;
            using pt::erase;
            using pt::unpack;
            iterator begin() const { return ++iterator(&this->_root, 0); }
            iterator end()   const { return iterator(&this->_root, 256); }
    };
}

#undef pt
#endif /* PTRIE_MAP_H */
