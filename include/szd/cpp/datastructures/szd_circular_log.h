/** \file
 * Log that only allows appending, reading an complete resets.
 * */
#pragma once
#ifndef SZD_CIRCULAR_LOG_H
#define SZD_CIRCULAR_LOG_H

#include "szd/cpp/datastructures/szd_buffer.h"
#include "szd/cpp/datastructures/szd_log.h"
#include "szd/cpp/szd_channel.h"
#include "szd/cpp/szd_channel_factory.h"
#include "szd/cpp/szd_status.h"
#include "szd/szd.h"

#include <string>

namespace SimpleZNSDeviceNamespace {
class SZDCircularLog : public SZDLog {
public:
  SZDCircularLog(SZDChannelFactory *channel_factory, const DeviceInfo &info,
                 const uint64_t min_zone_head, const uint64_t max_zone_head);
  ~SZDCircularLog() override;
  SZDStatus Append(const std::string string, uint64_t *lbas = nullptr) override;
  SZDStatus Append(const char *data, const size_t size,
                   uint64_t *lbas = nullptr, bool alligned = true) override;
  SZDStatus Append(const SZDBuffer &buffer, uint64_t *lbas = nullptr) override;
  SZDStatus Append(const SZDBuffer &buffer, size_t addr, size_t size,
                   uint64_t *lbas = nullptr, bool alligned = true) override;
  SZDStatus Read(char *data, uint64_t lba, uint64_t size,
                 bool alligned = true) override;
  SZDStatus Read(SZDBuffer *buffer, uint64_t lba, uint64_t size,
                 bool alligned = true) override;
  SZDStatus Read(SZDBuffer *buffer, size_t addr, size_t size, uint64_t lba,
                 bool alligned = true) override;
  SZDStatus ConsumeTail(uint64_t begin_lba, uint64_t end_lba);
  SZDStatus ResetAll() override;
  SZDStatus RecoverPointers() override;
  bool Empty() const override { return write_head_ == min_zone_head_; }
  bool SpaceLeft(const size_t size) const override;
  bool IsValidReadAddress(const uint64_t addr, const uint64_t lbas) const;

private:
  // log
  uint64_t zone_head_;
  uint64_t zone_tail_;
  // references
  SZDChannel *channel_;
};
} // namespace SimpleZNSDeviceNamespace

#endif
