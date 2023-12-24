/*
 * CYCoroutine License
 * -----------
 *
 * CYCoroutine is licensed under the terms of the MIT license reproduced below.
 * This means that CYCoroutine is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 *
 *
 * ===============================================================================
 *
 * Copyright (C) 2023-2024 ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ===============================================================================
 */
/*
 * AUTHORS:  ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
 * VERSION:  1.0.0
 * PURPOSE:  A cross-platform efficient and stable Coroutine library.
 * CREATION: 2023.04.15
 * LCHANGE:  2023.04.15
 * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
 */

#ifndef __CY_COROUTINE_CORO_HPP__
#define __CY_COROUTINE_CORO_HPP__

#include "CYCoroutine/CYCoroutineDefine.hpp"
#include "CYCoroutine/Common/CYDebugString.hpp"
#include "CYCoroutine/Results/CYPromises.hpp"
#include "CYCoroutine/Executors/CYDerivableExecutor.hpp"
#include "CYCoroutine/Executors/CYInlineExecutor.hpp"
#include "CYCoroutine/Executors/CYManualExecutor.hpp"
#include "CYCoroutine/Executors/CYThreadExecutor.hpp"
#include "CYCoroutine/Executors/CYThreadPoolExecutor.hpp"
#include "CYCoroutine/Executors/CYWorkerThreadExecutor.hpp"
#include "CYCoroutine/Results/CYGenerator.hpp"
#include "CYCoroutine/Results/CYLazyResult.hpp"
#include "CYCoroutine/Results/CYMakeResult.hpp"
#include "CYCoroutine/Results/CYResult.hpp"
#include "CYCoroutine/Results/CYResumeOn.hpp"
#include "CYCoroutine/Results/CYSharedResult.hpp"
#include "CYCoroutine/Results/CYSharedResultAwaitable.hpp"
#include "CYCoroutine/Results/CYWhenResult.hpp"
#include "CYCoroutine/Engine/CYCoroutineEngine.hpp"
#include "CYCoroutine/Threads/CYAsyncCondition.hpp"
#include "CYCoroutine/Threads/CYAsyncLock.hpp"
#include "CYCoroutine/Timers/CYTimer.hpp"
#include "CYCoroutine/Timers/CYTimerQueue.hpp"

#endif //__CY_COROUTINE_CORO_HPP__
