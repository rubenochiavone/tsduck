//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher text Stealing (CTS) mode, alternative 3.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {
    //!
    //!  Cipher text Stealing (CTS) mode, alternative 3.
    //!  @ingroup crypto
    //!
    //!  Several incompatible designs of CTS exist. This one implements the
    //!  description of "ECB ciphertext stealing" in
    //!  http://en.wikipedia.org/wiki/Ciphertext_stealing
    //!
    //!  CTS can process a residue. The plain text and cipher text sizes must be
    //!  greater than the block size of the underlying block cipher.
    //!
    //!  @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class CTS3: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(CTS3);
    public:
        //!
        //! Constructor.
        //!
        CTS3() : CipherChainingTemplate<CIPHER>(0, 0, 1) {}

        // Implementation of CipherChaining interface.
        virtual size_t minMessageSize() const override;
        virtual bool residueAllowed() const override;

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    protected:
        // Implementation of BlockCipher interface.
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template<class CIPHER>
size_t ts::CTS3<CIPHER>::minMessageSize() const
{
    return this->block_size + 1;
}

template<class CIPHER>
bool ts::CTS3<CIPHER>::residueAllowed() const
{
    return true;
}

template<class CIPHER>
ts::UString ts::CTS3<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-CTS3";
}


//----------------------------------------------------------------------------
// Encryption in CTS3 mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS3<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr ||
        this->work.size() < this->block_size ||
        plain_length <= this->block_size ||
        cipher_maxsize < plain_length)
    {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    const uint8_t* pt = reinterpret_cast<const uint8_t*> (plain);
    uint8_t* ct = reinterpret_cast<uint8_t*> (cipher);

    // Process in ECB mode, except the last 2 blocks
    while (plain_length > 2 * this->block_size) {
        if (!this->algo->encrypt(pt, this->block_size, ct, this->block_size)) {
            return false;
        }
        ct += this->block_size;
        pt += this->block_size;
        plain_length -= this->block_size;
    }

    // Process final two blocks.
    assert(plain_length > this->block_size);
    const size_t residue_size = plain_length - this->block_size;

    if (!this->algo->encrypt(pt, this->block_size, this->work.data(), this->block_size)) {
        return false;
    }
    ::memcpy(ct + this->block_size, this->work.data(), residue_size);  // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data(), pt + this->block_size, residue_size);  // Flawfinder: ignore: memcpy()
    if (!this->algo->encrypt(this->work.data(), this->block_size, ct, this->block_size)) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS3 mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS3<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (this->algo == nullptr ||
        this->work.size() < this->block_size ||
        cipher_length <= this->block_size ||
        plain_maxsize < cipher_length)
    {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* ct = reinterpret_cast<const uint8_t*> (cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*> (plain);

    // Process in ECB mode, except the last 2 blocks
    while (cipher_length > 2 * this->block_size) {
        if (!this->algo->decrypt (ct, this->block_size, pt, this->block_size)) {
            return false;
        }
        ct += this->block_size;
        pt += this->block_size;
        cipher_length -= this->block_size;
    }

    // Process final two blocks.
    assert(cipher_length > this->block_size);
    const size_t residue_size = cipher_length - this->block_size;

    if (!this->algo->decrypt(ct, this->block_size, this->work.data(), this->block_size)) {
        return false;
    }
    ::memcpy(pt + this->block_size, this->work.data(), residue_size);  // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data(), ct + this->block_size, residue_size);  // Flawfinder: ignore: memcpy()
    if (!this->algo->decrypt(this->work.data(), this->block_size, pt, this->block_size)) {
        return false;
    }

    return true;
}
