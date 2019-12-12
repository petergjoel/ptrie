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
// Created by Peter G. Jensen on 12/9/16.

#include <iostream>
#include <ptrie/ptrie.h>
#include <stdlib.h>
#include <sparsehash/sparse_hash_set>
#include <sparsehash/dense_hash_set>
#include <tbb/concurrent_unordered_set.h>
#include <random>
#include <ptrie/ptrie_stable.h>
#include <chrono>
#include <unordered_set>
#include "MurmurHash2.h"
#include "utils.h"
using namespace ptrie;

ptrie::binarywrapper_t rand_data(size_t seed, size_t maxsize, size_t minsize = sizeof(size_t), size_t mv = 256)
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
        data.raw()[j] = (uchar) (rand() % mv);
    }
    // make sure everything is unique
    for (size_t j = 1; j <= sizeof(size_t); ++j) {
        data.raw()[size - j] = ((uchar *) &seed)[j - 1];
    }
    return data;
}


struct wrapper_t
{
    binarywrapper_t data;
    uint64_t _hash;

    bool operator <(const wrapper_t& other) const
    {
        if(_hash == other._hash)
        {
            if(data.size() == other.data.size()) {
                return memcmp(data.const_raw(), other.data.const_raw(), data.size()) > 0;
            }
            else
            {
                return data.size() < other.data.size();
            }
        }
        return _hash < other._hash;
    }

};

template<typename T>
void set_insert(T& set, size_t elements, size_t seed, size_t bytes, double deletes, double read_rate, size_t mv)
{
    std::default_random_engine generator(seed);
    std::uniform_real_distribution<double> dist;
    std::uniform_int_distribution<int>  rem(0, elements);

    std::default_random_engine read_generator(seed);
    std::normal_distribution<double> read_dist(read_rate, read_rate / 2.0);
    std::uniform_int_distribution<int> read_el(0, elements);


    wrapper_t w;
    auto start = std::chrono::system_clock::now();
    for(size_t i = 0; i < elements; ++i)
    {
        w.data = rand_data(seed + i, bytes, bytes, mv);
        w._hash = MurmurHash64A(w.data.raw(), w.data.size(), seed);
        if(w._hash == 0) w._hash += 1;
        else if (w._hash == std::numeric_limits<uint64_t>::max()) w._hash -= 1;
        set.insert(w);

        if(read_rate > 0.0)
        {
            int reads = std::round(read_dist(read_generator));
            for(int r = 0; r < reads; ++r)
            {
                size_t el = read_el(read_generator);
                w.data = rand_data(seed + el, bytes, bytes, mv);
                w._hash = MurmurHash64A(w.data.raw(), w.data.size(), seed);
                if(w._hash == 0) w._hash += 1;
                else if (w._hash == std::numeric_limits<uint64_t>::max()) w._hash -= 1;
                set.count(w);
                w.data.release();
            }
        }

/*        if(dist(generator) < deletes)
        {
            size_t el = rem(generator) % elements;
            w.data = rand_data(seed + el, bytes, bytes, mv);
            w._hash = MurmurHash64A(w.data.raw(), w.data.size(), seed);
            if(w._hash == 0) w._hash += 1;
            else if (w._hash == std::numeric_limits<uint64_t>::max()) w._hash -= 1;
            auto it = set.find(w);
            if(it != set.end())
                set.erase(it);
            w.data.release();
        }*/
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "COMPLETED IN " << (0.001*elapsed.count()) << " SECONDS " << std::endl;
    w.data = binarywrapper_t();
}

template<typename T> 
void set_insert_ptrie(T& set, size_t elements, size_t seed, size_t bytes, double deletes, double read_rate, size_t mv)
{
    std::default_random_engine del_generator(seed);
    std::uniform_real_distribution<double> del_dist;
    std::uniform_int_distribution<int>  del_el(0, elements);

    std::default_random_engine read_generator(seed);
    std::normal_distribution<double> read_dist(read_rate, read_rate / 2.0);
    std::uniform_int_distribution<int> read_el(0, elements);
    auto start = std::chrono::system_clock::now();

    for(size_t i = 0; i < elements; ++i)
    {
        auto data = rand_data(seed + i, bytes, bytes, mv);
        set.insert(data);
        data.release();

        if(read_rate > 0.0)
        {
            int reads = std::round(read_dist(read_generator));
            for(int r = 0; r < reads; ++r)
            {
                size_t el = read_el(read_generator);
                data = rand_data(seed + el, bytes, bytes, mv);
                set.exists(data);
                data.release();
            }
        }
        /*if(dist(generator) < deletes)
        {
            size_t el = rem(generator) % elements;
            auto torem = rand_data(seed + el, bytes, bytes, mv);
            set.erase(torem);
            torem.release();
        }*/
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "COMPLETED IN " << (0.001*elapsed.count()) << " SECONDS " << std::endl;
}


struct hasher_o
{
    uint64_t operator()(const wrapper_t& w) const
    {
        return w._hash;
    }
};

struct equal_o
{
    bool operator()(const wrapper_t& w1, const wrapper_t& w2) const
    {
        if(w1._hash == w2._hash)
        {
            return w1.data == w2.data;
        }
        else
        {
            return false;
        }
    }
};

int main(int argc, const char** argv)
{
    if(argc < 3 || argc > 8)
    {
        std::cout << "usage : <ptrie/std/sparse/dense> <number elements> <?seed> <?number of bytes> <?delete ratio> <?read rate> <?max byte val>" << std::endl;
        exit(-1);
    }

    const char* type = argv[1];
    size_t elements = 1024;
    size_t seed = 0;
    size_t bytes = 16;
    double deletes = 0.0;
    double read_rate = 0.0;
    size_t maxval = 256;

    read_arg<size_t>(argv[2], elements, "Error in <number of elements>", "%zu");
    if(argc > 3) read_arg<size_t>(argv[3], seed, "Error in <seed>", "%zu");
    if(argc > 4) read_arg<size_t>(argv[4], bytes, "Error in <bytes>", "%zu");
    if(argc > 5) read_arg<double>(argv[5], deletes, "Error in <delete ratio>", "%lf");
    if(argc > 6) read_arg<double>(argv[6], read_rate, "Error in <read rate>", "%lf");
    if(argc > 7) read_arg<size_t>(argv[7], maxval, "Error in <max byte val>", "%zu");

    if(strcmp(type, "ptrie") == 0)
    {
        print_settings(type, elements, seed, bytes, deletes, read_rate, maxval);
        set<> set;
        set_insert_ptrie(set, elements, seed, bytes, deletes, read_rate, maxval);
    }
    else if (strcmp(type, "std") == 0) {
        print_settings(type, elements, seed, bytes, deletes, read_rate, maxval);
        std::unordered_set<wrapper_t, hasher_o, equal_o> set;
        set_insert(set, elements, seed, bytes, deletes, read_rate, maxval);
    }
    else if(strcmp(type, "tbb") == 0)
    {
        print_settings(type, elements, seed, bytes, deletes, read_rate, maxval);
        tbb::concurrent_unordered_set<wrapper_t, hasher_o, equal_o> set;
        set_insert(set, elements, seed, bytes, deletes, read_rate, maxval);
    }
    else if(strcmp(type, "sparse") == 0)
    {
        print_settings(type, elements, seed, bytes, deletes, read_rate, maxval);
        google::sparse_hash_set<wrapper_t, hasher_o, equal_o> set(elements/10);
        set_insert(set, elements, seed, bytes, deletes, read_rate, maxval);
    }
    else if(strcmp(type, "dense") == 0)
    {
        print_settings(type, elements, seed, bytes, deletes, read_rate, maxval);
        google::dense_hash_set<wrapper_t, hasher_o, equal_o> set(elements/10);
        wrapper_t empty;
        empty._hash = 0;
        wrapper_t del;
        del._hash = std::numeric_limits<uint64_t>::max();
        set.set_empty_key(empty);
        if(deletes > 0.0) set.set_deleted_key(del);
        set_insert(set, elements, seed, bytes, deletes, read_rate, maxval);
    }
    else
    {
        std::cerr << "ERROR IN TYPE, ONLY VALUES ALLOWED : ptrie, std, sparse, dense, tbb" << std::endl;
        exit(-1);
    }

    return 0;
}
