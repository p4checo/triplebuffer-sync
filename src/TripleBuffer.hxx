//============================================================================
// Name        : TripleBuffer.hxx
// Author      : André Pacheco Neves
// Version     : 1.0 (27/01/13)
// Copyright   : Copyright (c) 2013, André Pacheco Neves
//               All rights reserved.
//
//               Redistribution and use in source and binary forms, with or without
//               modification, are permitted provided that the following conditions are met:
//               	* Redistributions of source code must retain the above copyright
//               	  notice, this list of conditions and the following disclaimer.
//               	* Redistributions in binary form must reproduce the above copyright
//               	  notice, this list of conditions and the following disclaimer in the
//               	  documentation and/or other materials provided with the distribution.
//               	* Neither the name of the <organization> nor the
//               	  names of its contributors may be used to endorse or promote products
//               	  derived from this software without specific prior written permission.
//
//               THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//               ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//               WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//               DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
//               DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//               (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//               LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//               ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//               (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//               SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// Description : Template class for a TripleBuffer as a concurrency mechanism, using atomic operations
// Credits     : http://remis-thoughts.blogspot.pt/2012/01/triple-buffering-as-concurrency_30.html
//============================================================================

#ifndef TRIPLEBUFFER_HXX_
#define TRIPLEBUFFER_HXX_

#include <atomic>

using namespace std;

template <typename T>
class TripleBuffer
{

public:

	TripleBuffer<T>();
	TripleBuffer<T>(const T& init);

	// non-copyable behavior
	TripleBuffer<T>(const TripleBuffer<T>&) = delete;
	TripleBuffer<T>& operator=(const TripleBuffer<T>&) = delete;

	T snap() const; // get the current snap to read
	void write(const T newT); // write a new value
	bool newSnap(); // swap to the latest value, if any
	void flipWriter(); // flip writer positions dirty / clean

	T readLast(); // wrapper to read the last available element (newSnap + snap)
	void update(T newT); // wrapper to update with a new element (write + flipWriter)

private:

	bool isNewWrite(uint_fast8_t flags); // check if the newWrite bit is 1
	uint_fast8_t swapSnapWithClean(uint_fast8_t flags); // swap Snap and Clean indexes
	uint_fast8_t newWriteSwapCleanWithDirty(uint_fast8_t flags); // set newWrite to 1 and swap Clean and Dirty indexes

	// 8 bit flags are (unused) (new write) (2x dirty) (2x clean) (2x snap)
	// newWrite   = (flags & 0x40)
	// dirtyIndex = (flags & 0x30) >> 4
	// cleanIndex = (flags & 0xC) >> 2
	// snapIndex  = (flags & 0x3)
	mutable atomic_uint_fast8_t flags;

	T buffer[3];
};

// include implementation in header since it is a template

template <typename T>
TripleBuffer<T>::TripleBuffer(){

	T dummy = T();

	buffer[0] = dummy;
	buffer[1] = dummy;
	buffer[2] = dummy;

	flags.store(0x6, std::memory_order_relaxed); // initially dirty = 0, clean = 1 and snap = 2
}

template <typename T>
TripleBuffer<T>::TripleBuffer(const T& init){

	buffer[0] = init;
	buffer[1] = init;
	buffer[2] = init;

	flags.store(0x6, std::memory_order_relaxed); // initially dirty = 0, clean = 1 and snap = 2
}

template <typename T>
T TripleBuffer<T>::snap() const{

	return buffer[flags.load(std::memory_order_consume) & 0x3]; // read snap index
}

template <typename T>
void TripleBuffer<T>::write(const T newT){

	buffer[(flags.load(std::memory_order_consume) & 0x30) >> 4] = newT; // write into dirty index
}

template <typename T>
bool TripleBuffer<T>::newSnap(){

	uint_fast8_t flagsNow;
	uint_fast8_t newFlags;
	do {
		flagsNow = flags.load(std::memory_order_consume);
		if( !isNewWrite(flagsNow) ) // nothing new, no need to swap
			return false;
		newFlags = swapSnapWithClean(flagsNow);
	} while(!flags.compare_exchange_weak(flagsNow,
			newFlags,
			memory_order_release,
			memory_order_consume));

	return true;
}

template <typename T>
void TripleBuffer<T>::flipWriter(){

	uint_fast8_t flagsNow;
	uint_fast8_t newFlags;
	do {
		flagsNow = flags.load(std::memory_order_consume);
		newFlags = newWriteSwapCleanWithDirty(flagsNow);
	} while(!flags.compare_exchange_weak(flagsNow,
			newFlags,
			memory_order_release,
			memory_order_consume));
}

template <typename T>
T TripleBuffer<T>::readLast(){
	newSnap(); // get most recent value
	return snap(); // return it
}

template <typename T>
void TripleBuffer<T>::update(T newT){
	write(newT); // write new value
	flipWriter(); // change dirty/clean buffer positions for the next update
}

template <typename T>
bool TripleBuffer<T>::isNewWrite(uint_fast8_t flags){
	// check if the newWrite bit is 1
	return ((flags & 0x40) != 0);
}

template <typename T>
uint_fast8_t TripleBuffer<T>::swapSnapWithClean(uint_fast8_t flags){
	// swap snap with clean
	return (flags & 0x30) | ((flags & 0x3) << 2) | ((flags & 0xC) >> 2);
}

template <typename T>
uint_fast8_t TripleBuffer<T>::newWriteSwapCleanWithDirty(uint_fast8_t flags){
	// set newWrite bit to 1 and swap clean with dirty
	return 0x40 | ((flags & 0xC) << 2) | ((flags & 0x30) >> 2) | (flags & 0x3);
}

#endif /* TRIPLEBUFFER_HXX_ */
