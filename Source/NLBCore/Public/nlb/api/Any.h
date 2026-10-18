#pragma once
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// Собственная реализация Any для совместимости с macOS High Sierra.
// Без RTTI: модуль Unreal под Android собирается с -fno-rtti, иначе сборщику нужны typeinfo
// классов движка, которых в его сборке нет. Тип различается адресом статической метки,
// своей у каждого T
class Any {
private:
    template<typename T>
    static const void* typeKey() {
        static const char key = 0;
        return &key;
    }

    // Базовый класс для хранения любого типа
    struct Base {
        virtual ~Base() {}
        virtual std::unique_ptr<Base> clone() const = 0;
        virtual const void* type() const = 0;
    };
    
    // Шаблонный класс для конкретного типа
    template<typename T>
    struct Derived : Base {
        T value;
        
        explicit Derived(const T& val) : value(val) {}
        
        std::unique_ptr<Base> clone() const override {
            return std::make_unique<Derived<T>>(value);
        }
        
        const void* type() const override {
            return typeKey<T>();
        }
    };
    
    std::unique_ptr<Base> ptr;
    
public:
    // Конструкторы
    Any() : ptr(nullptr) {}
    
    template<typename T>
    Any(const T& value) : ptr(std::make_unique<Derived<T>>(value)) {}
    
    // Конструктор копирования
    Any(const Any& other) : ptr(other.ptr ? other.ptr->clone() : nullptr) {}
    
    // Конструктор перемещения
    Any(Any&& other) noexcept : ptr(std::move(other.ptr)) {}
    
    // Операторы присваивания
    Any& operator=(const Any& other) {
        if (this != &other) {
            ptr = other.ptr ? other.ptr->clone() : nullptr;
        }
        return *this;
    }
    
    Any& operator=(Any&& other) noexcept {
        if (this != &other) {
            ptr = std::move(other.ptr);
        }
        return *this;
    }
    
    // Шаблонный оператор присваивания
    template<typename T>
    Any& operator=(const T& value) {
        ptr = std::make_unique<Derived<T>>(value);
        return *this;
    }
    
    // Приведение типа с проверкой
    template<typename T>
    T cast() const {
        if (!ptr || ptr->type() != typeKey<T>()) {
            throw std::runtime_error(ptr ? "Bad any cast: wrong type" : "Bad any cast: empty");
        }
        return static_cast<Derived<T>*>(ptr.get())->value;
    }
    
    // Безопасное приведение типа (возвращает указатель)
    template<typename T>
    const T* cast_ptr() const {
        if (!ptr || ptr->type() != typeKey<T>()) {
            return nullptr;
        }
        return &(static_cast<Derived<T>*>(ptr.get())->value);
    }
    
    // Проверка на пустоту
    bool empty() const {
        return ptr == nullptr;
    }
    
    // Очистка
    void reset() {
        ptr.reset();
    }
    
    // Проверка типа
    template<typename T>
    bool is_type() const {
        return ptr && ptr->type() == typeKey<T>();
    }
};

// Пример использования:
/*
int main() {
    Any a1 = 42;
    Any a2 = 3.14159;
    Any a3 = std::string("Hello");
    Any a4 = true;
    
    std::cout << StringHelper::anyToString(a1) << std::endl;     // "42"
    std::cout << StringHelper::anyToString(a2) << std::endl;     // "3.142"
    std::cout << StringHelper::anyToString(a2, 6) << std::endl;  // "3.141590"
    std::cout << StringHelper::anyToString(a3) << std::endl;     // "Hello"
    std::cout << StringHelper::anyToString(a4) << std::endl;     // "true"
    
    return 0;
}
*/