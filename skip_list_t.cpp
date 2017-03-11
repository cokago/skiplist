
#include <string>
#include <iostream>
#include <assert.h>
#include <unistd.h>

#include "skip_list.h"

int main() {
    zskiplist* zsl = nullptr;
    zsl = zslCreate();

    int total = 0;

    int n = 1000;
    long _ele = 100001L;
    for(int i=0; i<n; i++) {
        long ele = _ele++;
        double score = -1.0*ele;
        zslInsert(zsl, score, ele);
        total++;
    }
    constexpr int LIMIT = 100;

    for(long k=0; k<1000000L; k++) {
        n = rand()%200;
        for(int i=0; i<n; i++) {
            long ele = _ele++;
            double score = -1.0*ele;
            zslInsert(zsl, score, ele);
            total++;
        }
        if(k % 2 == 0) {
            // 旧
            n = rand() % 200;
            double score100100 = -1*_ele+n;
            zskiplistNode* node100099 = zslNodeGT(zsl, score100100);
            if(node100099 == NULL) {
                std::cerr<<"NOT FOUND GT "<<score100100<<" header="<<zsl->header->level[0].forward->score<<" tail="<<zsl->tail->score<<std::endl;
                continue;
            }
            //std::cerr<<"LOCATE GT "<<score100100<<" -> "<<node100099->ele<<" "<<node100099->score<<std::endl;
            long lst1[LIMIT];
            double score_limit = score100100+LIMIT+1;
            //n = zslNodeCopy(node100099, LIMIT, score_limit, lst1);
            n = zslNodeCopy(node100099, LIMIT, lst1);
            std::cerr<<"OLD: "<<n<<" ("<<-score100100<<","<<-score_limit<<"), ["<<lst1[0]<<","<<lst1[n-1]<<"] total="<<total<<" header="<<zsl->header->level[0].forward->score<<" tail="<<zsl->tail->score<<std::endl;
            //for(int i=0; i<n; i++) {
            //    std::cerr<<i<<" "<<lst1[i]<<std::endl;
            //}
        } else {
            // 新
            n = rand() % 100;
            double score100100 = -1*_ele+n;
            zskiplistNode* node100101 = zslNodeLT(zsl, score100100);
            if(node100101 == NULL) {
                std::cerr<<"NOT FOUND LT "<<score100100<<" header="<<zsl->header->level[0].forward->score<<" tail="<<zsl->tail->score<<std::endl;
                continue;
            }
            long lst2[LIMIT];
            //std::cerr<<"LOCATE LT "<<score100100<<" -> "<<node100101->ele<<" "<<node100101->score<<std::endl;
            double score_limit = score100100-LIMIT+90;
            //n = zslNodeRevCopy(zsl, node100101, LIMIT, score_limit, lst2);
            n = zslNodeRevCopy(node100101, LIMIT, score_limit, lst2);
            std::cerr<<"NEW: "<<n<<" ("<<-score100100<<","<<-score_limit<<"), ["<<lst2[0]<<", "<<lst2[n-1]<<"] total="<<total<<" header="<<zsl->header->level[0].forward->score<<" tail="<<zsl->tail->score<<std::endl;
            //for(int i=0; i<n; i++) {
            //    std::cerr<<i<<" "<<lst2[i]<<std::endl;
            //}
        }
        std::cerr<<std::endl<<std::endl;
        usleep(100);
        //getchar();
    }
    zslFree(zsl);
}
