#pragma once

#include <BaseStateClass.hpp>

template<class T> requires (std::derived_from<T, TAGBaseState> && !std::same_as<T, TAGBaseState>) void TAGBaseState::addState(const std::string& name) {
	states.emplace(name, std::make_unique<T>());
}