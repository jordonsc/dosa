#pragma once

#include <Arduino.h>

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

    bool operator==(const LinkedListItem& rhs) const
    {
        if (data_type != rhs.data_type) {
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
        return !(rhs == *this);
    }

    LinkedListItemType getDataType() const
    {
        return data_type;
    }

   private:
    void* value;
    LinkedListItemType data_type;
};

template <class KeyClass>
class LinkedList
{
   public:
    template <class T>
    void set(KeyClass key, T&& value)
    {
        items[key] = LinkedListItem(std::forward(value));
    }

    void set(KeyClass key, LinkedListItem item)
    {
        items[key] = std::move(item);
    }

    [[nodiscard]] LinkedListItem& operator[](KeyClass key)
    {
        return items[key];
    }

    [[nodiscard]] LinkedListItem const& operator[](KeyClass key) const
    {
        return items[key];
    }

    void clear(KeyClass key)
    {
        items[key] = LinkedListItem();
    }

    [[nodiscard]] bool has(KeyClass key) const
    {
        return items[key].getDataType() != LinkedListItemType::NONE;
    }

   protected:
    LinkedListItem items[sizeof(KeyClass) * 8];
};

}  // namespace dosa