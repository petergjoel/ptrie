/* 
 * Copyright (C) 2016  Peter G. Jensen <root@petergjoel.dk>
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
// Created by Peter G. Jensen on 12/12/16.

#ifndef PTRIE_UTILS_H
#define PTRIE_UTILS_H

#include <ptrie/binarywrapper.h>

using namespace ptrie;

template<typename T, typename G>
void try_insert(T& trie, G generator, size_t N)
{
    for(size_t i = 0; i < N; ++i)
    {
        binarywrapper_t data = generator(i);
        auto exists = trie.exists(data.raw(), data.size());
        BOOST_REQUIRE_MESSAGE(!exists.first,
                              "FAILED ON INSERT " << i << " BIN " << data << " ");

        auto inserted = trie.insert(data.raw(), data.size());
        BOOST_REQUIRE_MESSAGE(inserted.first,
                              "EXIST FAILED FOR " << i << " BIN " << data << " ");
        data.release();
    }

    for(size_t i = 0; i < N; ++i) {
        binarywrapper_t data = generator(i);
        auto exists = trie.exists(data.raw(), data.size());
        BOOST_REQUIRE_MESSAGE(exists.first,
                              "POST EXIST CHECK FAILED FOR " << i << " BIN " << data << " ");
        data.release();
    }

}

ptrie::binarywrapper_t rand_data(size_t seed, size_t maxsize, size_t minsize = sizeof(size_t))
{
    assert(minsize >= sizeof(size_t));
    srand(seed);
    // pick size between 0 and maxsize
    size_t size = minsize != maxsize ?
                  minsize + rand() % (maxsize - minsize) :
                  minsize;

    binarywrapper_t data(size * 8);
    // fill in random data
    for (size_t j = 0; j < size; ++j) {
        data.raw()[j] = (uchar) rand();
    }
    // make sure everything is unique
    for (size_t j = 1; j <= sizeof(size_t); ++j) {
        data.raw()[size - j] = ((uchar *) &seed)[j - 1];
    }
    return data;
}
#endif //PTRIE_UTILS_H
