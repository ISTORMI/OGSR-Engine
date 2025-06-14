#pragma once

template <class T, const int dim>
class svector
{
public:
    typedef size_t size_type;
    typedef T value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef value_type& reference;
    typedef const value_type& const_reference;

private:
    value_type array[dim]{};
    u32 count;

public:
    svector() : count(0) {}
    svector(iterator p, int c) { assign(p, c); }

    IC iterator begin() { return array; }
    IC iterator end() { return array + count; }
    IC const_iterator begin() const { return array; }
    IC const_iterator end() const { return array + count; }
    IC u32 size() const { return count; }
    IC void clear() { count = 0; }
    IC void resize(int c)
    {
        R_ASSERT(c <= dim);
        count = c;
    }
    /*IC void reserve(int c) {}*/

    IC void push_back(value_type e)
    {
        R_ASSERT(count < dim); //  <!!!
        array[count++] = e;
    }
    IC void pop_back()
    {
        R_ASSERT(count);
        count--;
    }

    IC reference operator[](u32 id)
    {
        R_ASSERT(id < count);
        return array[id];
    }
    IC const_reference operator[](u32 id) const
    {
        R_ASSERT(id < count);
        return array[id];
    }

    IC reference front() { return array[0]; }
    IC reference back()
    {
        R_ASSERT(count);
        return array[count - 1];
    }
    IC reference last()
    {
        R_ASSERT(count < dim);
        return array[count];
    }
    IC const_reference front() const { return array[0]; }
    IC const_reference back() const
    {
        R_ASSERT(count);
        return array[count - 1];
    }
    IC const_reference last() const
    {
        R_ASSERT(count < dim);
        return array[count];
    }
    IC void inc() { count++; }
    IC bool empty() const { return 0 == count; }

    IC void erase(u32 id)
    {
        R_ASSERT(id < count);
        count--;
        for (u32 i = id; i < count; i++)
            array[i] = array[i + 1];
    }
    IC void erase(iterator it) { erase(u32(it - begin())); }

    IC void insert(u32 id, reference V)
    {
        R_ASSERT(id < count);
        for (int i = count; i > int(id); i--)
            array[i] = array[i - 1];
        count++;
        array[id] = V;
    }
    IC void assign(const_iterator p, int c)
    {
        R_ASSERT(c > 0 && c <= dim);
        CopyMemory(array, p, c * sizeof(value_type));
        count = c;
    }
    IC BOOL equal(const svector<value_type, dim>& base) const
    {
        if (size() != base.size())
            return FALSE;
        for (u32 cmp = 0; cmp < size(); cmp++)
            if ((*this)[cmp] != base[cmp])
                return FALSE;
        return TRUE;
    }
};
