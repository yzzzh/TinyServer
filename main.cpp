#include <unistd.h>
#include "models/MyProactor.h"

int main(int argc,char** argv) {
    short port = 12345;
    bool _daemon = false;
    int ch;
    while((ch = getopt(argc,argv,"p:d")) != -1){
        switch (ch){
            case 'd':
                _daemon = true;
                break;
            case 'p':
                port = atoi(optarg);
                break;
        }
    }

    if(_daemon)
        daemon(0,0);

    MyProactor proactor("0.0.0.0",port);

    if(!proactor.init()){
        cout << "fail to init proactor" << endl;
        return 1;
    }

    proactor.run();

    //proactor.uninit();

    return 0;
}
