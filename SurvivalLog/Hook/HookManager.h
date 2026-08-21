#pragma once
#ifndef HookManager_a
#define HookManager_a

#include <map>
#include "detours.h"
#include <functional>
#pragma comment( lib, "Hook/detours-x64.lib")

#define CALL_ORIGIN(function, ...) \
	HookManager::call(function, __func__, __VA_ARGS__)

class HookManager {
public:
	template <typename Fn>
	static void install(Fn func, Fn handler) {
		hookList.insert(std::pair<void*, void*>((void*)handler, (void*)func));
		enable(func, handler);
		holderMap[reinterpret_cast<void*>(handler)] = reinterpret_cast<void*>(func);
	}

	template <typename Fn>
	[[nodiscard]] static Fn getOrigin(Fn handler, const char* callerName = nullptr) noexcept {
		for (size_t i = 0; i <= 5; i++) {
			if (holderMap.count(reinterpret_cast<void*>(handler)) == 0) {
				if (i < 5) {
					Sleep(1000);
					continue;
				}
				printf("Origin not found for handler: %s. Maybe racing bug.", callerName == nullptr ? "<Unknown>" : callerName);

				ExitProcess(2);
				return nullptr;
			}
			else
			{
				break;
			}
		}
		return reinterpret_cast<Fn>(holderMap[reinterpret_cast<void*>(handler)]);
	}

	template <typename Fn>
	[[nodiscard]] static void detach(Fn handler) noexcept {
		disable(handler);
		holderMap.erase(reinterpret_cast<void*>(handler));
		hookList.erase(reinterpret_cast<void*>(handler));
	}

	template <typename RType, typename... Params>
	[[nodiscard]] static RType call(RType(*handler)(Params...), const char* callerName = nullptr, Params... params) {
		auto origin = getOrigin(handler, callerName);
		if (origin != nullptr)
			return origin(params...);

		return RType();
	}

	static void detachAll() noexcept {
		for (const auto& [key, value] : holderMap) {
			disable(key);
		}
		holderMap.clear();
		hookList.clear();
	}

	static std::map<void*, void*> getHookList() {
		return hookList;
	}
	template<typename RetType, typename... Args>
	static void HookFunction(void* offset, RetType(*hookFunc)(Args...)) {
		// 定义函数指针类型
		using FuncType = RetType(*)(Args...);
		FuncType targetFuncPtr = reinterpret_cast<FuncType>(offset);

		// 安装钩子
		install(reinterpret_cast<void*>(targetFuncPtr), reinterpret_cast<void*>(hookFunc));
	}
	template<typename RetType, typename... Args>
	static void HookFunction(uint64_t offset, RetType(*hookFunc)(Args...)) {
		// 定义函数指针类型
		using FuncType = RetType(*)(Args...);
		FuncType targetFuncPtr = reinterpret_cast<FuncType>(offset);

		// 安装钩子
		install(reinterpret_cast<void*>(targetFuncPtr), reinterpret_cast<void*>(hookFunc));
	}
	template<typename RetType, typename... Args>
	static void HookFunction(void* offset, std::function<RetType(Args...)> hookFunc) {
		// 定义函数指针类型
		using FuncType = RetType(*)(Args...);
		FuncType targetFuncPtr = reinterpret_cast<FuncType>(offset);

		// 安装钩子
		install(reinterpret_cast<void*>(targetFuncPtr), reinterpret_cast<void*>(hookFunc.target<RetType(*)(Args...)>()));
	}
private:
	inline static std::map<void*, void*> hookList{};
	inline static std::map<void*, void*> holderMap{};

	template <typename Fn>
	static void disable(Fn handler) {
		Fn origin = getOrigin(handler);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)origin, handler);
		DetourTransactionCommit();
	}

	template <typename Fn>
	static void enable(Fn& func, Fn handler) {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)func, handler);
		DetourTransactionCommit();
	}
};
#endif //HookManager_a
