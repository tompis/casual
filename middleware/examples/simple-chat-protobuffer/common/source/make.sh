#!/bin/sh
protoc --cpp_out=. chat.proto;
mv chat.pb.cc chat.pb.cpp;
