//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHash.h"
#include "tsByteBlock.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::Hash::~Hash()
{
#if defined(TS_WINDOWS)
    if (_hash != nullptr) {
        ::BCryptDestroyHash(_hash);
        _hash = nullptr;
    }
    if (_algo != nullptr) {
        ::BCryptCloseAlgorithmProvider(_algo, 0);
        _algo = nullptr;
    }
#endif
}


//----------------------------------------------------------------------------
// Reinitialize the computation of the hash.
//----------------------------------------------------------------------------

bool ts::Hash::init()
{
#if defined(TS_WINDOWS)

    // Open algorithm provider the first time.
    if (_algo == nullptr) {
        if (::BCryptOpenAlgorithmProvider(&_algo, algorithmId(), nullptr, 0) < 0) {
            return false;
        }
        // BCrypt needs a "hash object", get its size.
        ::DWORD objlength = 0;
        ::ULONG retsize = 0;
        if (::BCryptGetProperty(_algo, BCRYPT_OBJECT_LENGTH, ::PUCHAR(&objlength), sizeof(objlength), &retsize, 0) < 0) {
            return false;
        }
        // And allocate the "hash object" for the rest of the life of this Hash instance.
        _obj.resize(size_t(objlength));
    }
    // Terminate previous hash if not yet done.
    if (_hash != nullptr) {
        ::BCryptDestroyHash(_hash);
        _hash = nullptr;
    }
    // Create a new hash handle.
    if (::BCryptCreateHash(_algo, &_hash, _obj.data(), ::ULONG(_obj.size()), nullptr, 0, 0) < 0) {
        return false;
    }
    return true;

#else

    // Must be implemented in subclass.
    return false;    

#endif
}


//----------------------------------------------------------------------------
// Add some part of the message to hash. Can be called several times.
//----------------------------------------------------------------------------

bool ts::Hash::add(const void* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return true;
    }

#if defined(TS_WINDOWS)

    if (_hash == nullptr || ::BCryptHashData(_hash, ::PUCHAR(data), ::ULONG(size), 0) < 0) {
        return false;
    }
    return true;

#else

    // Must be implemented in subclass.
    return false;

#endif
}


//----------------------------------------------------------------------------
// Get the resulting hash value.
//----------------------------------------------------------------------------

bool ts::Hash::getHash(void* hash, size_t bufsize, size_t* retsize)
{
    const size_t hsize = hashSize();
    if (retsize != nullptr) {
        *retsize = hsize;
    }
    if (hash == nullptr || bufsize < hsize) {
        return false;
    }

#if defined(TS_WINDOWS)

    // We need to provide the expected size for the hash.
    if (_hash == nullptr || ::BCryptFinishHash(_hash, ::PUCHAR(hash), ::ULONG(hsize), 0) < 0) {
        return false;
    }
    // We cannot reuse the hash handler, destroy it.
    ::BCryptDestroyHash(_hash);
    _hash = nullptr;
    return true;

#else

    // Must be implemented in subclass.
    return false;

#endif
}


//----------------------------------------------------------------------------
// Compute a hash in one operation
//----------------------------------------------------------------------------

bool ts::Hash::hash(const void* data, size_t data_size, void* hash, size_t hash_maxsize, size_t* hash_retsize)
{
    return init() && add(data, data_size) && getHash(hash, hash_maxsize, hash_retsize);
}

bool ts::Hash::hash(const void* data, size_t data_size, ByteBlock& result)
{
    result.resize(hashSize());
    size_t retsize = 0;
    const bool ok = hash(data, data_size, &result[0], result.size(), &retsize);
    result.resize(ok ? retsize : 0);
    return ok;
}
