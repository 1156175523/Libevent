#include <event.h>
#include <stdio.h>

int main()
{
    char** method = event_get_supported_methods();
    int i = 0;
    while(*(method+i))
    {
        printf("%s\n",*(method+i));
        i++;
    }


    struct event_base *base = event_base_new();
    printf("default method :%s\n",event_base_get_method(base));
    event_base_free(base);
    return 0;
}
