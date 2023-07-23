#include "app.h"

#include <iostream>
#include <fmt/format.h>
#include "app.h"


int main()
{
    int ret = 0;
    if (Application::GetInstance()->Init())
    {
        try {
            ret = Application::GetInstance()->Exec();
        }
        catch (const std::exception& e)
        {
            fmt::print("app run error: {}\n", e.what());
            return -1;
        }
    }
    else{
        ret = -1;
    }


    return ret;
}