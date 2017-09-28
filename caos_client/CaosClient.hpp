//
// Created by Luke Taher on 11/07/2016.
//

#ifndef CSWCaosCLIENT_CaosCLIENT_HPP
#define CSWCaosCLIENT_CaosCLIENT_HPP

#pragma once

#include "Types.hpp"
#include "Block.hpp"

#include <vector>

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

class DataMap;

class CaosClient {
public:
	virtual ~CaosClient() = default;

	virtual void Write(size_t bid, blockData b, DataMap *map) = 0;
	virtual Block Read(size_t bid, DataMap *map) = 0;

	virtual size_t GetBlockCount() = 0;
	virtual size_t GetBlockSize() = 0;
	virtual size_t GetBlockDataSize() = 0;

	virtual bool WasSerialised() = 0;
};


#endif //CSWCaosCLIENT_CaosCLIENT_HPP
