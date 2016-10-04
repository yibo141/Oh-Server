#include "threadpool.h"

using namespace std;

class fuck
{
    public:
        void process()
        {
            cout << "in processing" << endl;
        }
};

static void* worker(void*)
{
    cout << "bitch" << endl;
}

int main()
{
    threadpool<fuck> *bitch;
    bitch = new threadpool<fuck>(8, 10000);
    bitch->add(new fuck());
    bitch->add(new fuck());
    return 0;

    /*
    pthread_t arr[4];

    for(int i = 0; i < 4; ++i)
    {
        cout << "create the " << i << "th thread" << endl;
        if(pthread_create(&arr[i], NULL, worker, NULL) != 0)
            cout << "fuck!!!" << endl;
    }
    return 0;
    */
}
