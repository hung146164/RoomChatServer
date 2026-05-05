#include<iostream>

template<typename T, int SIZE>
class Array
{
    private:
    T m_Array[SIZE];
    public:
    int GetSize() const {return SIZE;}
};

int main()
{
    Array<int,5> array;
    std::cout<<array.GetSize()<<std::endl;
}