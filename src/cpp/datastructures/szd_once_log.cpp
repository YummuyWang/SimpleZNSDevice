#include "szd/cpp/datastructures/szd_once_log.h"
#include "szd/cpp/szd_channel_factory.h"
#include "szd/szd.h"

#include <cassert>

namespace SimpleZNSDeviceNamespace {
SZDOnceLog::SZDOnceLog(SZDChannelFactory *channel_factory,
                       const DeviceInfo &info, const uint64_t min_zone_head,
                       const uint64_t max_zone_head)
    : SZDLog(channel_factory, info, min_zone_head, max_zone_head),
      zone_head_(min_zone_head) {
  channel_factory_->Ref();
  channel_factory_->register_channel(&channel_, min_zone_head / info.zone_size,
                                     max_zone_head_ / info.zone_size);
}

SZDOnceLog::~SZDOnceLog() {
  if (channel_ != nullptr) {
    channel_factory_->unregister_channel(channel_);
  }
  channel_factory_->Unref();
}

SZDStatus SZDOnceLog::Append(const char *data, const size_t size,
                             uint64_t *lbas, bool alligned) {
  SZDStatus s;
  if (!SpaceLeft(size)) {
    return SZDStatus::IOError;
  }
  uint64_t write_head_old = write_head_;
  s = channel_->DirectAppend(&write_head_, (void *)data, size, alligned);
  zone_head_ = (write_head_ / zone_size_) * zone_size_;
  if (lbas != nullptr) {
    *lbas = write_head_ - write_head_old;
  }
  return s;
}

SZDStatus SZDOnceLog::Append(const std::string string, uint64_t *lbas) {
  return Append(string.data(), string.size(), lbas);
}

SZDStatus SZDOnceLog::Append(const SZDBuffer &buffer, size_t addr, size_t size,
                             uint64_t *lbas, bool alligned) {
  SZDStatus s;
  if (!SpaceLeft(size)) {
    return SZDStatus::IOError;
  }
  uint64_t write_head_old = write_head_;
  s = channel_->FlushBufferSection(&write_head_, buffer, addr, size, alligned);
  zone_head_ = (write_head_ / zone_size_) * zone_size_;
  if (lbas != nullptr) {
    *lbas = write_head_ - write_head_old;
  }
  return s;
}

SZDStatus SZDOnceLog::Append(const SZDBuffer &buffer, uint64_t *lbas) {
  SZDStatus s;
  size_t size = buffer.GetBufferSize();
  if (!SpaceLeft(size)) {
    return SZDStatus::IOError;
  }
  uint64_t write_head_old = write_head_;
  s = channel_->FlushBuffer(&write_head_, buffer);
  zone_head_ = (write_head_ / zone_size_) * zone_size_;
  if (lbas != nullptr) {
    *lbas = write_head_ - write_head_old;
  }
  return s;
}

SZDStatus SZDOnceLog::Read(char *data, uint64_t lba, uint64_t size,
                           bool alligned) {
  // No need for bounds checks, this is already done by channel.
  return channel_->DirectRead(lba, data, size, alligned);
}

SZDStatus SZDOnceLog::Read(SZDBuffer *buffer, uint64_t lba, uint64_t size,
                           bool alligned) {
  // No need for bounds checks, this is already done by channel.
  return channel_->ReadIntoBuffer(lba, buffer, 0, size, alligned);
}

SZDStatus SZDOnceLog::Read(SZDBuffer *buffer, size_t addr, size_t size,
                           uint64_t lba, bool alligned) {
  // No need for bounds checks, this is already done by channel.
  return channel_->ReadIntoBuffer(lba, buffer, addr, size, alligned);
}

SZDStatus SZDOnceLog::ResetAll() {
  SZDStatus s;
  for (uint64_t slba = min_zone_head_; slba < max_zone_head_;
       slba += zone_size_) {
    s = channel_->ResetZone(slba);
    if (s != SZDStatus::Success) {
      return s;
    }
  }
  s = SZDStatus::Success;
  write_head_ = min_zone_head_;
  zone_head_ = min_zone_head_;
  return s;
}

SZDStatus SZDOnceLog::RecoverPointers() {
  SZDStatus s;
  uint64_t write_head = min_zone_head_;
  uint64_t zone_head = min_zone_head_, old_zone_head = min_zone_head_;
  for (uint64_t slba = min_zone_head_; slba < max_zone_head_;
       slba += zone_size_) {
    s = channel_->ZoneHead(slba, &zone_head);
    if (s != SZDStatus::Success) {
      return s;
    }
    // head is at last zone that is not empty
    if (zone_head > slba) {
      write_head = zone_head;
    }
    // end has been reached.
    if (write_head > min_zone_head_ && zone_head == slba) {
      break;
    }
  }
  write_head_ = write_head;
  zone_head_ = (write_head_ / zone_size_) * zone_size_;
  return SZDStatus::Success;
}

} // namespace SimpleZNSDeviceNamespace
