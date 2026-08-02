// Copyright (c) 2026, the Luxon Server contributors
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "global.hpp"

#ifdef LUXON_SERVER_ENABLE_COROUTINES
#include <AwaitableTask.hpp>

#define lco_return co_return
#define lco_await co_await
#define _lco_detached .detach()
#else
#define lco_return return
#define lco_await
#define _lco_detached
#endif

namespace server {
template <typename T = void>
using Awaitable =
#ifdef LUXON_SERVER_ENABLE_COROUTINES
    basiccoro::Task<T>;
#else
    T;
#endif
} // namespace server
