#pragma once

#include <BaseStateClass.hpp>

template<class C> void TAGBaseState::addState(const std::string& name) {
	static_assert(std::derived_from<std::remove_cvref_t<C>, TAGBaseState> && !std::same_as<C, TAGBaseState>, "C must be a class derived from TAGBaseState");
	states.emplace(name, std::make_unique<C>(name));
}