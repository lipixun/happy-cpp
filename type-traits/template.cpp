/* Author: lipixun
 * Created Time : 2024-06-26 13:39:44
 *
 * File Name: template.cpp
 * Description:
 *
 *  Template vs oop
 *
 */

#include <iostream>

//
// OOP
//

class BaseClass {
 public:
  virtual void run() = 0;
};

class SubClass0 : public BaseClass {
 public:
  virtual void run() override {
    // Override
    std::cout << "SubClass0::run" << std::endl;
  }
};

class SubClass1 : public BaseClass {
 public:
  virtual void run() override {
    // Override
    std::cout << "SubClass1::run" << std::endl;
  }
};

void run_class(BaseClass* ptr) { ptr->run(); }

//
// Template
//

template <size_t I>
void run() = delete;

template <>
void run<0>() {
  std::cout << "Template0::run" << std::endl;
}

template <>
void run<1>() {
  std::cout << "Template1::run" << std::endl;
}

template <size_t I>
void run_template() {
  run<I>();
}

//
// Test
//

int main() {
  SubClass0 c0;
  SubClass1 c1;
  run_class(&c0);
  run_class(&c1);

  run_template<0>();
  run_template<1>();
  return 0;
}

/*
Outputs:
SubClass0::run
SubClass1::run
Template0::run
Template1::run
*/
