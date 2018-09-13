#pragma once

#include "CoreClient.h"
#include "FIXInitiator.h"

#include <iostream>
#include <string>
#include <thread>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>

using namespace shift;

#define TESTSIZE 1
