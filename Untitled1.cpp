#include<iostream>
using namespace std;

class Student
{
public:
    string name;
    int id;
    float cgpa;
};

int main()
{
    Student s1;
    s1.name = "Sobhan";
    s1.id = 76;
    s1.cgpa = 2.66;

    cout <<s1.name << endl << s1.id <<endl << s1.cgpa <<endl;
}
