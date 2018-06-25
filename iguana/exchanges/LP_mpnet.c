
/******************************************************************************
 * Copyright © 2014-2018 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
//
//  LP_mpnet.c
//  marketmaker
//

int32_t LP_tradecommand(void *ctx,char *myipaddr,int32_t pubsock,cJSON *argjson,uint8_t *data,int32_t datalen);
int32_t LP_quoteparse(struct LP_quoteinfo *qp,cJSON *argjson);
void LP_gtc_addorder(struct LP_quoteinfo *qp);
char *LP_withdraw(struct iguana_info *coin,cJSON *argjson);

int32_t LP_mpnet_addorder(struct LP_quoteinfo *qp)
{
    uint64_t destvalue,destvalue2;
    if ( LP_iseligible(&destvalue,&destvalue2,0,qp->destcoin,qp->desttxid,qp->destvout,qp->destsatoshis,qp->feetxid,qp->feevout) > 0 )
    {
        LP_gtc_addorder(qp);
        return(0);
    }
    return(-1);
}

void LP_mpnet_init() // problem is coins not enabled yet
{
    char fname[1024],line[8192]; FILE *fp; struct LP_quoteinfo Q; cJSON *argjson;
    sprintf(fname,"%s/GTC/orders",GLOBAL_DBDIR), OS_compatible_path(fname);
    if ( (fp= fopen(fname,"rb+")) != 0 )
    {
        while ( fgets(line,sizeof(line),fp) > 0 )
        {
            if ( (argjson= cJSON_Parse(line)) != 0 )
            {
                if ( LP_quoteparse(&Q,argjson) == 0 )
                {
                    if ( LP_mpnet_addorder(&Q) == 0 )
                        printf("GTC %s",line);
                }
                free_json(argjson);
            }
        }
        fclose(fp);
    }
}

void LP_mpnet_send(int32_t localcopy,char *msg,int32_t sendflag,char *otheraddr)
{
    char fname[1024]; int32_t len; FILE *fp; char *hexstr,*retstr; cJSON *argjson,*outputs,*item; struct iguana_info *coin; uint8_t linebuf[8192];
    if ( localcopy != 0 )
    {
        sprintf(fname,"%s/GTC/orders",GLOBAL_DBDIR), OS_compatible_path(fname);
        if ( (fp= fopen(fname,"rb+")) == 0 )
            fp = fopen(fname,"wb+");
        else fseek(fp,0,SEEK_END);
        fprintf(fp,"%s\n",msg);
        fclose(fp);
    }
    if ( G.mpnet != 0 && sendflag != 0 && (coin= LP_coinfind("CHIPS")) != 0 && coin->inactive == 0 )
    {
        len = MMJSON_encode(linebuf,msg);
        //curl --url "http://127.0.0.1:7783" --data "{\"userpass\":\"$userpass\",\"method\":\"withdraw\",\"coin\":\"CHIPS\",\"outputs\":[{\"RHV2As4rox97BuE3LK96vMeNY8VsGRTmBj\":0.0001}],\"opreturn\":\"deadbeef\"}"
        if ( len > 0 )
        {
            argjson = cJSON_CreateObject();
            outputs = cJSON_CreateArray();
            if ( otheraddr != 0 && otheraddr[0] != 0 )
            {
                item = cJSON_CreateObject();
                jaddnum(item,otheraddr,dstr(10000));
                jaddi(outputs,item);
            }
            item = cJSON_CreateObject();
            jaddnum(item,coin->smartaddr,dstr(10000));
            jaddi(outputs,item);
            jadd(argjson,"outputs",outputs);
            jaddnum(argjson,"broadcast",1);
            jaddstr(argjson,"coin",coin->symbol);
            hexstr = calloc(1,len*2 + 1);
            init_hexbytes_noT(hexstr,linebuf,len);
            jaddstr(argjson,"opreturn",hexstr);
            free(hexstr);
            retstr = LP_withdraw(coin,argjson);
            free_json(argjson);
            if ( retstr != 0 )
            {
                //printf("mpnet.%s\n",retstr);
                free(retstr);
            }
        }
   }
}

// 2151978
// 404bc4ac452db07ed16376b3d7e77dbfc22b4a68f7243797125bd0d3bdddf8d1
// 893b46634456034a6d5d73b67026aa157b5e2addbfc6344dfbea6bae85f7dde0
// 717c7ef9de8504bd331f3ef52ed0a16ea0e070434e12cb4d63f5f081e999c43d dup

cJSON *LP_mpnet_get()
{
    return(0);
}

void LP_mpnet_check(void *ctx,char *myipaddr,int32_t pubsock)
{
    cJSON *argjson;
    while ( (argjson= LP_mpnet_get()) != 0 )
    {
        LP_tradecommand(ctx,myipaddr,pubsock,argjson,0,0);
        free_json(argjson);
    }
}