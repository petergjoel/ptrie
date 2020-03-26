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
 * File:   ptrie_stable.h
 * Author: Peter G. Jensen
 *
 *
 * Created on 06 October 2016, 13:51
 */

#ifndef PTRIE_STABLE_H
#define PTRIE_STABLE_H
#include "ptrie_stable.h"
namespace ptrie {

    template<
    typename KEY,
    typename T,
    uint16_t HEAPBOUND = 17,
    uint16_t SPLITBOUND = 128,
    uint8_t BSIZE = 8,
    size_t ALLOCSIZE = (1024 * 64),
    typename I = size_t>
    class map : protected __set_stable<KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, T, I> {
        static_assert(!std::is_same<void, T>::value, "T (map-to-type) must not be void");
        using pt = __set_stable<KEY, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, T, I>;
        using entrylist_t = typename pt::entrylist_t;
    public:
        using pt::__set_stable;
        using pt::exists;
        using pt::erase;
        using pt::unpack;
        using pt::insert;
        
        using node_t = typename pt::node_t;
        using fwdnode_t = typename pt::fwdnode_t;
        using pt::key_t; 
        static constexpr auto bsize = pt::bsize;
        static constexpr auto bdiv = pt::bdiv;
        static constexpr auto heapbound = HEAPBOUND;
        
        T& get_data(I index);
        T& operator[](KEY key)
        {
            return get_data(pt::insert(key).second);
        }
        
        T& operator[](std::pair<KEY*, size_t> key)
        {
            return get_data(pt::insert(key.first, key.second).second);
        }

        T& operator[](const std::vector<KEY>& key)
        {
            return get_data(pt::insert(key.data(), key.size()).second);
        }

        class iterator : public ordered_iterator<map, iterator>
        {
        public:
            iterator(const base_t* base, int16_t index, entrylist_t& entries)
            : ordered_iterator<map, iterator>(base, index), _entries(entries) {}
            I index() const {
                return static_cast<const typename pt::node_t*>(this->_node)
                        ->entries()[this->_index];
            }
            
            T& operator*() const {
                return _entries[index()]._data;
            }

            T& operator->() const {
                return _entries[index()]._data;
            }
        private:
            entrylist_t& _entries;
        };
        
        friend class iterator;
        
        iterator begin() const { return ++iterator(&this->_root, 0, *this->_entries.get()); }
        iterator end()   const { return iterator(&this->_root, 256, *this->_entries.get()); }

    };

    template<
    typename KEY,
    typename T,
    uint16_t HEAPBOUND,
    uint16_t SPLITBOUND,
    uint8_t BSIZE,
    size_t ALLOCSIZE,
    typename I>
    T&
    map<KEY, T, HEAPBOUND, SPLITBOUND, BSIZE, ALLOCSIZE, I>::get_data(I index) {
        typename pt::entry_t& ent = this->_entries->operator[](index);
        return ent._data;
    }
}
#undef pt
#endif /* PTRIE_STABLE_H */

