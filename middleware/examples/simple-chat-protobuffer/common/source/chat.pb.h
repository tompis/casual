// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: chat.proto

#ifndef PROTOBUF_chat_2eproto__INCLUDED
#define PROTOBUF_chat_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3000000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3000002 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace chat {

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_chat_2eproto();
void protobuf_AssignDesc_chat_2eproto();
void protobuf_ShutdownFile_chat_2eproto();

class ChatMessage;
class ChatRoom;
class ChatRooms;
class CreateChatRoom;

// ===================================================================

class ChatRoom : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:chat.ChatRoom) */ {
 public:
  ChatRoom();
  virtual ~ChatRoom();

  ChatRoom(const ChatRoom& from);

  inline ChatRoom& operator=(const ChatRoom& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const ChatRoom& default_instance();

  void Swap(ChatRoom* other);

  // implements Message ----------------------------------------------

  inline ChatRoom* New() const { return New(NULL); }

  ChatRoom* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ChatRoom& from);
  void MergeFrom(const ChatRoom& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const {
    return InternalSerializeWithCachedSizesToArray(false, output);
  }
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(ChatRoom* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string room_name = 1;
  void clear_room_name();
  static const int kRoomNameFieldNumber = 1;
  const ::std::string& room_name() const;
  void set_room_name(const ::std::string& value);
  void set_room_name(const char* value);
  void set_room_name(const char* value, size_t size);
  ::std::string* mutable_room_name();
  ::std::string* release_room_name();
  void set_allocated_room_name(::std::string* room_name);

  // optional string creator_nick = 2;
  void clear_creator_nick();
  static const int kCreatorNickFieldNumber = 2;
  const ::std::string& creator_nick() const;
  void set_creator_nick(const ::std::string& value);
  void set_creator_nick(const char* value);
  void set_creator_nick(const char* value, size_t size);
  ::std::string* mutable_creator_nick();
  ::std::string* release_creator_nick();
  void set_allocated_creator_nick(::std::string* creator_nick);

  // optional int32 room_id = 3;
  void clear_room_id();
  static const int kRoomIdFieldNumber = 3;
  ::google::protobuf::int32 room_id() const;
  void set_room_id(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:chat.ChatRoom)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::internal::ArenaStringPtr room_name_;
  ::google::protobuf::internal::ArenaStringPtr creator_nick_;
  ::google::protobuf::int32 room_id_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_chat_2eproto();
  friend void protobuf_AssignDesc_chat_2eproto();
  friend void protobuf_ShutdownFile_chat_2eproto();

  void InitAsDefaultInstance();
  static ChatRoom* default_instance_;
};
// -------------------------------------------------------------------

class ChatRooms : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:chat.ChatRooms) */ {
 public:
  ChatRooms();
  virtual ~ChatRooms();

  ChatRooms(const ChatRooms& from);

  inline ChatRooms& operator=(const ChatRooms& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const ChatRooms& default_instance();

  void Swap(ChatRooms* other);

  // implements Message ----------------------------------------------

  inline ChatRooms* New() const { return New(NULL); }

  ChatRooms* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ChatRooms& from);
  void MergeFrom(const ChatRooms& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const {
    return InternalSerializeWithCachedSizesToArray(false, output);
  }
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(ChatRooms* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // repeated .chat.ChatRoom chat_room = 1;
  int chat_room_size() const;
  void clear_chat_room();
  static const int kChatRoomFieldNumber = 1;
  const ::chat::ChatRoom& chat_room(int index) const;
  ::chat::ChatRoom* mutable_chat_room(int index);
  ::chat::ChatRoom* add_chat_room();
  ::google::protobuf::RepeatedPtrField< ::chat::ChatRoom >*
      mutable_chat_room();
  const ::google::protobuf::RepeatedPtrField< ::chat::ChatRoom >&
      chat_room() const;

  // @@protoc_insertion_point(class_scope:chat.ChatRooms)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::RepeatedPtrField< ::chat::ChatRoom > chat_room_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_chat_2eproto();
  friend void protobuf_AssignDesc_chat_2eproto();
  friend void protobuf_ShutdownFile_chat_2eproto();

  void InitAsDefaultInstance();
  static ChatRooms* default_instance_;
};
// -------------------------------------------------------------------

class CreateChatRoom : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:chat.CreateChatRoom) */ {
 public:
  CreateChatRoom();
  virtual ~CreateChatRoom();

  CreateChatRoom(const CreateChatRoom& from);

  inline CreateChatRoom& operator=(const CreateChatRoom& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const CreateChatRoom& default_instance();

  void Swap(CreateChatRoom* other);

  // implements Message ----------------------------------------------

  inline CreateChatRoom* New() const { return New(NULL); }

  CreateChatRoom* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const CreateChatRoom& from);
  void MergeFrom(const CreateChatRoom& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const {
    return InternalSerializeWithCachedSizesToArray(false, output);
  }
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(CreateChatRoom* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string room_name = 1;
  void clear_room_name();
  static const int kRoomNameFieldNumber = 1;
  const ::std::string& room_name() const;
  void set_room_name(const ::std::string& value);
  void set_room_name(const char* value);
  void set_room_name(const char* value, size_t size);
  ::std::string* mutable_room_name();
  ::std::string* release_room_name();
  void set_allocated_room_name(::std::string* room_name);

  // optional string creator_nick = 2;
  void clear_creator_nick();
  static const int kCreatorNickFieldNumber = 2;
  const ::std::string& creator_nick() const;
  void set_creator_nick(const ::std::string& value);
  void set_creator_nick(const char* value);
  void set_creator_nick(const char* value, size_t size);
  ::std::string* mutable_creator_nick();
  ::std::string* release_creator_nick();
  void set_allocated_creator_nick(::std::string* creator_nick);

  // @@protoc_insertion_point(class_scope:chat.CreateChatRoom)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::internal::ArenaStringPtr room_name_;
  ::google::protobuf::internal::ArenaStringPtr creator_nick_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_chat_2eproto();
  friend void protobuf_AssignDesc_chat_2eproto();
  friend void protobuf_ShutdownFile_chat_2eproto();

  void InitAsDefaultInstance();
  static CreateChatRoom* default_instance_;
};
// -------------------------------------------------------------------

class ChatMessage : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:chat.ChatMessage) */ {
 public:
  ChatMessage();
  virtual ~ChatMessage();

  ChatMessage(const ChatMessage& from);

  inline ChatMessage& operator=(const ChatMessage& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const ChatMessage& default_instance();

  void Swap(ChatMessage* other);

  // implements Message ----------------------------------------------

  inline ChatMessage* New() const { return New(NULL); }

  ChatMessage* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ChatMessage& from);
  void MergeFrom(const ChatMessage& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const {
    return InternalSerializeWithCachedSizesToArray(false, output);
  }
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(ChatMessage* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional int32 room_id = 1;
  void clear_room_id();
  static const int kRoomIdFieldNumber = 1;
  ::google::protobuf::int32 room_id() const;
  void set_room_id(::google::protobuf::int32 value);

  // optional string message = 2;
  void clear_message();
  static const int kMessageFieldNumber = 2;
  const ::std::string& message() const;
  void set_message(const ::std::string& value);
  void set_message(const char* value);
  void set_message(const char* value, size_t size);
  ::std::string* mutable_message();
  ::std::string* release_message();
  void set_allocated_message(::std::string* message);

  // optional string nick = 3;
  void clear_nick();
  static const int kNickFieldNumber = 3;
  const ::std::string& nick() const;
  void set_nick(const ::std::string& value);
  void set_nick(const char* value);
  void set_nick(const char* value, size_t size);
  ::std::string* mutable_nick();
  ::std::string* release_nick();
  void set_allocated_nick(::std::string* nick);

  // @@protoc_insertion_point(class_scope:chat.ChatMessage)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::internal::ArenaStringPtr message_;
  ::google::protobuf::internal::ArenaStringPtr nick_;
  ::google::protobuf::int32 room_id_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_chat_2eproto();
  friend void protobuf_AssignDesc_chat_2eproto();
  friend void protobuf_ShutdownFile_chat_2eproto();

  void InitAsDefaultInstance();
  static ChatMessage* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// ChatRoom

// optional string room_name = 1;
inline void ChatRoom::clear_room_name() {
  room_name_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& ChatRoom::room_name() const {
  // @@protoc_insertion_point(field_get:chat.ChatRoom.room_name)
  return room_name_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatRoom::set_room_name(const ::std::string& value) {
  
  room_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:chat.ChatRoom.room_name)
}
inline void ChatRoom::set_room_name(const char* value) {
  
  room_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:chat.ChatRoom.room_name)
}
inline void ChatRoom::set_room_name(const char* value, size_t size) {
  
  room_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:chat.ChatRoom.room_name)
}
inline ::std::string* ChatRoom::mutable_room_name() {
  
  // @@protoc_insertion_point(field_mutable:chat.ChatRoom.room_name)
  return room_name_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ChatRoom::release_room_name() {
  // @@protoc_insertion_point(field_release:chat.ChatRoom.room_name)
  
  return room_name_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatRoom::set_allocated_room_name(::std::string* room_name) {
  if (room_name != NULL) {
    
  } else {
    
  }
  room_name_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), room_name);
  // @@protoc_insertion_point(field_set_allocated:chat.ChatRoom.room_name)
}

// optional string creator_nick = 2;
inline void ChatRoom::clear_creator_nick() {
  creator_nick_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& ChatRoom::creator_nick() const {
  // @@protoc_insertion_point(field_get:chat.ChatRoom.creator_nick)
  return creator_nick_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatRoom::set_creator_nick(const ::std::string& value) {
  
  creator_nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:chat.ChatRoom.creator_nick)
}
inline void ChatRoom::set_creator_nick(const char* value) {
  
  creator_nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:chat.ChatRoom.creator_nick)
}
inline void ChatRoom::set_creator_nick(const char* value, size_t size) {
  
  creator_nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:chat.ChatRoom.creator_nick)
}
inline ::std::string* ChatRoom::mutable_creator_nick() {
  
  // @@protoc_insertion_point(field_mutable:chat.ChatRoom.creator_nick)
  return creator_nick_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ChatRoom::release_creator_nick() {
  // @@protoc_insertion_point(field_release:chat.ChatRoom.creator_nick)
  
  return creator_nick_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatRoom::set_allocated_creator_nick(::std::string* creator_nick) {
  if (creator_nick != NULL) {
    
  } else {
    
  }
  creator_nick_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), creator_nick);
  // @@protoc_insertion_point(field_set_allocated:chat.ChatRoom.creator_nick)
}

// optional int32 room_id = 3;
inline void ChatRoom::clear_room_id() {
  room_id_ = 0;
}
inline ::google::protobuf::int32 ChatRoom::room_id() const {
  // @@protoc_insertion_point(field_get:chat.ChatRoom.room_id)
  return room_id_;
}
inline void ChatRoom::set_room_id(::google::protobuf::int32 value) {
  
  room_id_ = value;
  // @@protoc_insertion_point(field_set:chat.ChatRoom.room_id)
}

// -------------------------------------------------------------------

// ChatRooms

// repeated .chat.ChatRoom chat_room = 1;
inline int ChatRooms::chat_room_size() const {
  return chat_room_.size();
}
inline void ChatRooms::clear_chat_room() {
  chat_room_.Clear();
}
inline const ::chat::ChatRoom& ChatRooms::chat_room(int index) const {
  // @@protoc_insertion_point(field_get:chat.ChatRooms.chat_room)
  return chat_room_.Get(index);
}
inline ::chat::ChatRoom* ChatRooms::mutable_chat_room(int index) {
  // @@protoc_insertion_point(field_mutable:chat.ChatRooms.chat_room)
  return chat_room_.Mutable(index);
}
inline ::chat::ChatRoom* ChatRooms::add_chat_room() {
  // @@protoc_insertion_point(field_add:chat.ChatRooms.chat_room)
  return chat_room_.Add();
}
inline ::google::protobuf::RepeatedPtrField< ::chat::ChatRoom >*
ChatRooms::mutable_chat_room() {
  // @@protoc_insertion_point(field_mutable_list:chat.ChatRooms.chat_room)
  return &chat_room_;
}
inline const ::google::protobuf::RepeatedPtrField< ::chat::ChatRoom >&
ChatRooms::chat_room() const {
  // @@protoc_insertion_point(field_list:chat.ChatRooms.chat_room)
  return chat_room_;
}

// -------------------------------------------------------------------

// CreateChatRoom

// optional string room_name = 1;
inline void CreateChatRoom::clear_room_name() {
  room_name_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& CreateChatRoom::room_name() const {
  // @@protoc_insertion_point(field_get:chat.CreateChatRoom.room_name)
  return room_name_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CreateChatRoom::set_room_name(const ::std::string& value) {
  
  room_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:chat.CreateChatRoom.room_name)
}
inline void CreateChatRoom::set_room_name(const char* value) {
  
  room_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:chat.CreateChatRoom.room_name)
}
inline void CreateChatRoom::set_room_name(const char* value, size_t size) {
  
  room_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:chat.CreateChatRoom.room_name)
}
inline ::std::string* CreateChatRoom::mutable_room_name() {
  
  // @@protoc_insertion_point(field_mutable:chat.CreateChatRoom.room_name)
  return room_name_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* CreateChatRoom::release_room_name() {
  // @@protoc_insertion_point(field_release:chat.CreateChatRoom.room_name)
  
  return room_name_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CreateChatRoom::set_allocated_room_name(::std::string* room_name) {
  if (room_name != NULL) {
    
  } else {
    
  }
  room_name_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), room_name);
  // @@protoc_insertion_point(field_set_allocated:chat.CreateChatRoom.room_name)
}

// optional string creator_nick = 2;
inline void CreateChatRoom::clear_creator_nick() {
  creator_nick_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& CreateChatRoom::creator_nick() const {
  // @@protoc_insertion_point(field_get:chat.CreateChatRoom.creator_nick)
  return creator_nick_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CreateChatRoom::set_creator_nick(const ::std::string& value) {
  
  creator_nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:chat.CreateChatRoom.creator_nick)
}
inline void CreateChatRoom::set_creator_nick(const char* value) {
  
  creator_nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:chat.CreateChatRoom.creator_nick)
}
inline void CreateChatRoom::set_creator_nick(const char* value, size_t size) {
  
  creator_nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:chat.CreateChatRoom.creator_nick)
}
inline ::std::string* CreateChatRoom::mutable_creator_nick() {
  
  // @@protoc_insertion_point(field_mutable:chat.CreateChatRoom.creator_nick)
  return creator_nick_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* CreateChatRoom::release_creator_nick() {
  // @@protoc_insertion_point(field_release:chat.CreateChatRoom.creator_nick)
  
  return creator_nick_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CreateChatRoom::set_allocated_creator_nick(::std::string* creator_nick) {
  if (creator_nick != NULL) {
    
  } else {
    
  }
  creator_nick_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), creator_nick);
  // @@protoc_insertion_point(field_set_allocated:chat.CreateChatRoom.creator_nick)
}

// -------------------------------------------------------------------

// ChatMessage

// optional int32 room_id = 1;
inline void ChatMessage::clear_room_id() {
  room_id_ = 0;
}
inline ::google::protobuf::int32 ChatMessage::room_id() const {
  // @@protoc_insertion_point(field_get:chat.ChatMessage.room_id)
  return room_id_;
}
inline void ChatMessage::set_room_id(::google::protobuf::int32 value) {
  
  room_id_ = value;
  // @@protoc_insertion_point(field_set:chat.ChatMessage.room_id)
}

// optional string message = 2;
inline void ChatMessage::clear_message() {
  message_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& ChatMessage::message() const {
  // @@protoc_insertion_point(field_get:chat.ChatMessage.message)
  return message_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatMessage::set_message(const ::std::string& value) {
  
  message_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:chat.ChatMessage.message)
}
inline void ChatMessage::set_message(const char* value) {
  
  message_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:chat.ChatMessage.message)
}
inline void ChatMessage::set_message(const char* value, size_t size) {
  
  message_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:chat.ChatMessage.message)
}
inline ::std::string* ChatMessage::mutable_message() {
  
  // @@protoc_insertion_point(field_mutable:chat.ChatMessage.message)
  return message_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ChatMessage::release_message() {
  // @@protoc_insertion_point(field_release:chat.ChatMessage.message)
  
  return message_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatMessage::set_allocated_message(::std::string* message) {
  if (message != NULL) {
    
  } else {
    
  }
  message_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), message);
  // @@protoc_insertion_point(field_set_allocated:chat.ChatMessage.message)
}

// optional string nick = 3;
inline void ChatMessage::clear_nick() {
  nick_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& ChatMessage::nick() const {
  // @@protoc_insertion_point(field_get:chat.ChatMessage.nick)
  return nick_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatMessage::set_nick(const ::std::string& value) {
  
  nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:chat.ChatMessage.nick)
}
inline void ChatMessage::set_nick(const char* value) {
  
  nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:chat.ChatMessage.nick)
}
inline void ChatMessage::set_nick(const char* value, size_t size) {
  
  nick_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:chat.ChatMessage.nick)
}
inline ::std::string* ChatMessage::mutable_nick() {
  
  // @@protoc_insertion_point(field_mutable:chat.ChatMessage.nick)
  return nick_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ChatMessage::release_nick() {
  // @@protoc_insertion_point(field_release:chat.ChatMessage.nick)
  
  return nick_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ChatMessage::set_allocated_nick(::std::string* nick) {
  if (nick != NULL) {
    
  } else {
    
  }
  nick_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), nick);
  // @@protoc_insertion_point(field_set_allocated:chat.ChatMessage.nick)
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS
// -------------------------------------------------------------------

// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace chat

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_chat_2eproto__INCLUDED
