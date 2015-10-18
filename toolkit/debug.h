#ifndef DEBUG_H
#define DEBUG_H

//#define NEW(x) new x; printf("new at %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
#define NEW(x) new x;
//#define NEWV(x, y) new x; printf("[Thread %d] new at %s:%s:%d\n", y, __FILE__, __FUNCTION__, __LINE__);
#define NEWV(x, y) new x;

//#define DELETE(x) delete x; printf("delete at %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
#define DELETE(x) delete x;
//#define DELETEV(x, y) delete x; printf("[Thread %d] delete at %s:%s:%d\n", y, __FILE__, __FUNCTION__, __LINE__);
#define DELETEV(x, y) delete x;

//#define DELETETAB(x) delete [] x; printf("delete [] at %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
#define DELETETAB(x) delete [] x;
//#define DELETETABV(x, y) delete [] x; printf("[Thread %d] delete [] at %s:%s:%d\n", y, __FILE__, __FUNCTION__, __LINE__);
#define DELETETABV(x, y) delete [] x;

#endif
