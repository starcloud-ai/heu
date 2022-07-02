// Copyright 2022 Ant Group Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "heu/library/algorithms/paillier_zahlen/encryptor.h"

#include "fmt/format.h"

#include "heu/library/algorithms/util/montgomery_math.h"

namespace heu::lib::algorithms::paillier_z {

Encryptor::Encryptor(PublicKey pk) : pk_(std::move(pk)) {}
Encryptor::Encryptor(const Encryptor &from) : Encryptor(from.pk_) {}

MPInt Encryptor::GetRn() const {
  MPInt r;
  MPInt::RandomRoundUp(pk_.key_size_ / 2, &r);

  // (h_s_)^r
  MPInt out;
  pk_.m_space_->PowMod(*pk_.hs_table_, r, &out);
  return out;
}

Ciphertext Encryptor::EncryptZero() const { return Ciphertext(GetRn()); }

template <bool audit>
Ciphertext Encryptor::EncryptImpl(const MPInt &m,
                                  std::string *audit_str) const {
  YASL_ENFORCE(m.CompareAbs(pk_.PlaintextBound()) < 0,
               "message number out of range, message={}, max (abs)={}",
               m.ToHexString(), pk_.PlaintextBound());

  // Note: g^m = (1 + n)^m = (1 + n*m) mod n^2
  // It is also correct when m is negative
  MPInt gm = (pk_.n_ * m).IncrOne();  // no need mod

  pk_.m_space_->MapIntoMSpace(&gm);
  Ciphertext ct;
  auto rn = GetRn();
  pk_.m_space_->MulMod(gm, rn, &ct.c_);
  if constexpr (audit) {
    YASL_ENFORCE(audit_str != nullptr);
    *audit_str = fmt::format("p:{},rn:{},c:{}", m.ToHexString(),
                             rn.ToHexString(), ct.c_.ToHexString());
  }
  return ct;
}

Ciphertext Encryptor::Encrypt(const MPInt &m) const { return EncryptImpl(m); }

std::pair<Ciphertext, std::string> Encryptor::EncryptWithAudit(
    const MPInt &m) const {
  std::string audit_str;
  auto c = EncryptImpl<true>(m, &audit_str);
  return {c, audit_str};
}

}  // namespace heu::lib::algorithms::paillier_z
