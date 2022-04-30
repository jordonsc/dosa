#pragma once

#include <Arduino.h>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace dosa {

enum class LinkedListItemType : uint8_t
{
    NONE,
    PTR,
    BOOL,
    STRING,
    UINT8,
    UINT16,
    UINT32,
    INT8,
    INT16,
    INT32,
    FLOAT32,
    FLOAT64
};

template <class KeyClass>
class LinkedListItem final
{
   public:
    LinkedListItem()
    {
        data_type = LinkedListItemType::NONE;
        value = nullptr;
    }

    LinkedListItem(LinkedListItem const& item)
    {
        key = item.key;
        data_type = item.data_type;

        switch (item.data_type) {
            case LinkedListItemType::NONE:
                break;
            case LinkedListItemType::PTR:
                value = item.value;
                break;
            case LinkedListItemType::BOOL:
                value = new uint8_t(item.getUInt8());
                break;
            case LinkedListItemType::STRING:
                value = new String(item.getString());
                break;
            case LinkedListItemType::UINT8:
                value = new uint8_t(item.getUInt8());
                break;
            case LinkedListItemType::UINT16:
                value = new uint16_t(item.getUInt16());
                break;
            case LinkedListItemType::UINT32:
                value = new uint32_t(item.getUInt32());
                break;
            case LinkedListItemType::INT8:
                value = new int8_t(item.getInt8());
                break;
            case LinkedListItemType::INT16:
                value = new int16_t(item.getInt16());
                break;
            case LinkedListItemType::INT32:
                value = new int32_t(item.getInt32());
                break;
            case LinkedListItemType::FLOAT32:
                value = new float(item.getFloat32());
                break;
            case LinkedListItemType::FLOAT64:
                value = new double(item.getFloat64());
                break;
        }
    }

    friend void swap(LinkedListItem& first, LinkedListItem& second) noexcept
    {
        using std::swap;

        swap(first.value, second.value);
        swap(first.data_type, second.data_type);
    }

    LinkedListItem& operator=(LinkedListItem other)
    {
        swap(*this, other);

        return *this;
    }

    LinkedListItem(LinkedListItem&& other) noexcept : LinkedListItem()
    {
        swap(*this, other);
    }

    LinkedListItem(void* v)
    {
        data_type = LinkedListItemType::PTR;
        value = v;
    }

    LinkedListItem(bool v)
    {
        data_type = LinkedListItemType::BOOL;
        value = new uint8_t(v ? 1 : 0);
    }

    LinkedListItem(String const& v)
    {
        data_type = LinkedListItemType::STRING;
        value = new String(v);
    }

    LinkedListItem(char const* v)
    {
        data_type = LinkedListItemType::STRING;
        value = new String(v);
    }

    LinkedListItem(uint8_t v)
    {
        data_type = LinkedListItemType::UINT8;
        value = new uint8_t(v);
    }

    LinkedListItem(uint16_t v)
    {
        data_type = LinkedListItemType::UINT16;
        value = new uint16_t(v);
    }

    LinkedListItem(uint32_t v)
    {
        data_type = LinkedListItemType::UINT32;
        value = new uint32_t(v);
    }

    LinkedListItem(int8_t v)
    {
        data_type = LinkedListItemType::INT8;
        value = new int8_t(v);
    }

    LinkedListItem(int16_t v)
    {
        data_type = LinkedListItemType::INT16;
        value = new int16_t(v);
    }

    LinkedListItem(int32_t v)
    {
        data_type = LinkedListItemType::INT32;
        value = new int32_t(v);
    }

    LinkedListItem(float v)
    {
        data_type = LinkedListItemType::FLOAT32;
        value = new float(v);
    }

    LinkedListItem(double v)
    {
        data_type = LinkedListItemType::FLOAT64;
        value = new double(v);
    }

    virtual ~LinkedListItem()
    {
        switch (data_type) {
            case LinkedListItemType::NONE:
            case LinkedListItemType::PTR:
                break;
            case LinkedListItemType::STRING:
                delete (String*)value;
                break;
            case LinkedListItemType::BOOL:
            case LinkedListItemType::UINT8:
                delete (uint8_t*)value;
                break;
            case LinkedListItemType::UINT16:
                delete (uint16_t*)value;
                break;
            case LinkedListItemType::UINT32:
                delete (uint32_t*)value;
                break;
            case LinkedListItemType::INT8:
                delete (int8_t*)value;
                break;
            case LinkedListItemType::INT16:
                delete (int16_t*)value;
                break;
            case LinkedListItemType::INT32:
                delete (int32_t*)value;
                break;
            case LinkedListItemType::FLOAT32:
                delete (float*)value;
                break;
            case LinkedListItemType::FLOAT64:
                delete (double*)value;
                break;
        }
    }

    [[nodiscard]] void* getPointer() const
    {
        return value;
    }

    [[nodiscard]] bool getBool() const
    {
        return bool((uint8_t*)value);
    }

    [[nodiscard]] String const& getString() const
    {
        return *(String*)value;
    }

    [[nodiscard]] uint8_t getUInt8() const
    {
        return *(uint8_t*)value;
    }

    [[nodiscard]] uint16_t getUInt16() const
    {
        return *(uint16_t*)value;
    }

    [[nodiscard]] uint32_t getUInt32() const
    {
        return *(uint32_t*)value;
    }

    [[nodiscard]] int8_t getInt8() const
    {
        return *(int8_t*)value;
    }

    [[nodiscard]] int16_t getInt16() const
    {
        return *(int16_t*)value;
    }

    [[nodiscard]] int32_t getInt32() const
    {
        return *(int32_t*)value;
    }

    [[nodiscard]] float getFloat32() const
    {
        return *(float*)value;
    }

    [[nodiscard]] float getFloat64() const
    {
        return *(double*)value;
    }

    void setKey(KeyClass v)
    {
        key = v;
    }

    [[nodiscard]] KeyClass getKey() const
    {
        return key;
    }

    bool operator==(const LinkedListItem& rhs) const
    {
        if (data_type != rhs.data_type || key != rhs.key) {
            return false;
        }

        switch (data_type) {
            case LinkedListItemType::NONE:
                return true;
            case LinkedListItemType::PTR:
                return false;
            case LinkedListItemType::BOOL:
                return getBool() == rhs.getBool();
            case LinkedListItemType::STRING:
                return getString() == rhs.getString();
            case LinkedListItemType::UINT8:
                return getUInt8() == rhs.getUInt8();
            case LinkedListItemType::UINT16:
                return getUInt16() == rhs.getUInt16();
            case LinkedListItemType::UINT32:
                return getUInt32() == rhs.getUInt32();
            case LinkedListItemType::INT8:
                return getInt8() == rhs.getInt8();
            case LinkedListItemType::INT16:
                return getInt16() == rhs.getInt16();
            case LinkedListItemType::INT32:
                return getInt32() == rhs.getInt32();
            case LinkedListItemType::FLOAT32:
                return getFloat32() == rhs.getFloat32();
            case LinkedListItemType::FLOAT64:
                return getFloat64() == rhs.getFloat64();
        }
    }

    bool operator!=(LinkedListItem const& rhs) const
    {
        return !operator==(rhs);
    }

    LinkedListItemType getDataType() const
    {
        return data_type;
    }

   private:
    KeyClass key;
    void* value;
    LinkedListItemType data_type;
};

#define LL_UTYPE(key) static_cast<typename std::underlying_type<KeyClass>::type>(key)

template <class KeyClass>
class LinkedList
{
   public:
    using ItemClass = LinkedListItem<KeyClass>;

    struct Iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = ItemClass;
        using pointer = ItemClass*;
        using reference = ItemClass&;

        Iterator(pointer ptr, pointer last) : item_ptr(ptr), end(last) {}

        ItemClass const& operator*() const
        {
            return *item_ptr;
        }

        ItemClass& operator*()
        {
            return *item_ptr;
        }

        ItemClass* operator->()
        {
            return item_ptr;
        }

        ItemClass const* operator->() const
        {
            return item_ptr;
        }

        Iterator& operator++()
        {
            do {
                ++item_ptr;

                if (item_ptr == end) {
                    break;
                }
            } while (item_ptr->getDataType() == LinkedListItemType::NONE);

            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++item_ptr;
            return tmp;
        }

        bool operator==(Iterator const& rhs)
        {
            return item_ptr == rhs.item_ptr;
        };

        bool operator!=(Iterator const& rhs)
        {
            return item_ptr != rhs.item_ptr;
        };

       private:
        pointer item_ptr;
        pointer end;
    };

    Iterator begin()
    {
        auto size = getSize();
        return Iterator(&items[0], &items[size - 1]);
    }

    Iterator end()
    {
        auto size = getSize();
        return Iterator(&items[size - 1], &items[size - 1]);
    }

    void set(KeyClass key, ItemClass item)
    {
        items[LL_UTYPE(key)] = std::move(item);
        items[LL_UTYPE(key)].setKey(key);
    }

    [[nodiscard]] ItemClass& operator[](KeyClass key)
    {
        return items[LL_UTYPE(key)];
    }

    [[nodiscard]] ItemClass const& operator[](KeyClass key) const
    {
        return items[LL_UTYPE(key)];
    }

    void clear(KeyClass key)
    {
        items[LL_UTYPE(key)] = ItemClass();
    }

    [[nodiscard]] bool has(KeyClass key) const
    {
        return items[LL_UTYPE(key)].getDataType() != LinkedListItemType::NONE;
    }

   protected:
    size_t getSize() const
    {
        return 2 ^ (sizeof(KeyClass) * 8);
    }

    ItemClass items[2 ^ (sizeof(KeyClass) * 8)];
};

}  // namespace dosa