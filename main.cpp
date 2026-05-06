#include<iostream>

class Object
{
    private:
    public:
    
    virtual void SpeakName()=0;
};
class Player : Object
{
    public:
    void SpeakName()
    {
        std::cout<<"Player"<<std::endl;
    }
};


template<typename T, int SIZE>
class Array
{
    private:
    T m_Array[SIZE];
    public:
    int GetSize() const {return SIZE;}
};

void h(Object* o)
{
    o->SpeakName();
}
int main()
{
    
}