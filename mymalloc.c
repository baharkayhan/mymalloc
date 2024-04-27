/* 
 * File: mymalloc.c
 * Authors: Bahar Kayhan, Beyza Nur Caglar
 * Date: 28 Nisan 2024
 * Description: Bu dosya bir memory allocation ornegidir. 
 */

#include "mymalloc.h"
#include <stdio.h>
#include <unistd.h>

Block *heap_genislet(size_t boyut);

int main() {
    int input;
    printf("Kullanici input'u: ");
    scanf("%d",&input);
    heap_genislet(input);
    printheap();
    return 0;
}


/* Boyutu en yakin 16 nin katina yuvarla */
size_t yuvarla(size_t boyut) {
    return (boyut + 15) & ~15;
}


/* Heap i sbrk kullanarak genisletme islemi */
Block *heap_genislet(size_t boyut) {
    Block *yeni_blok ;
    size_t toplam_boyut = yuvarla(boyut + sizeof(Block) + sizeof(Tag) ); /* Gereken boyutu hesapla */
    
    /* heap i genislet */
    yeni_blok = sbrk(toplam_boyut);
    if (yeni_blok == NULL) {        /* sbrk hatasi varsa kontrol et */
            perror("Error in extending heap");
            return NULL;
    }
    /* yeni blogu baslat */
    yeni_blok->info.size = toplam_boyut - sizeof(Block) - sizeof(Tag);
    yeni_blok->info.isfree = 1;
    yeni_blok->next = NULL;
    yeni_blok->prev = NULL;
    
 /* eger heapteki ilk bloksa heap_endi gunceller  */
    if (heap_start == NULL) {
        heap_start = yeni_blok;
    }
    heap_end = (Block *)((char *)yeni_blok + toplam_boyut);    
    return yeni_blok;
}


/* Free listte bos bir blok bulma */
Block *bos_blok_bul(size_t boyut) {
    Block *current = free_list; 
    /* Uygun bir blok bulmak icin free listi dolas */
    while (current != NULL) {
        if (current->info.size >= boyut) {
            return current;
        }
        current = next_block_in_freelist(current);
    }
    return NULL;
}


/* Bellek ayirma islemi */
void *mymalloc(size_t size) { 
    size_t yuvarlanmis_boyut = yuvarla(size); /* Boyutu 16nin en yakin katina yuvarla */
    Block *blok;
    
    /* Free listte bos bir blok bul */
    blok = bos_blok_bul(yuvarlanmis_boyut);
    if (blok == NULL) {    /* Uygun blok yoksa heapi genisletir */
        blok = heap_genislet(yuvarlanmis_boyut);
        if (blok == NULL)
            return NULL;    /* Allocation basarisiz oldu */
    }
    else {   /* Uygun bir blok bulunursa, gerekrse bol */
        blok = split_block(blok, yuvarlanmis_boyut);
    }
    /* Blogu tahsis edilmis olarak isaretle */
    blok->info.isfree = 0;
    
    return blok->data;
}


/* Bellegi bosaltma islemi */
void myfree(void *p) {
    if (p == NULL){
        return;     /* Eger pointer NULL ise hicbir sey yapma */
    }
    Block *blok = (Block *)((char *)p - sizeof(Tag) - sizeof(Block));     
    /* Blogu bos olarak isaretle */
    blok->info.isfree = 1;
    
    /* Mumkun ise komsu bloklarla birles */
    blok = left_coalesce(blok);
    blok = right_coalesce(blok);
}


/* Bir blogu gerekenden buyukse bolme islemi */
Block *split_block(Block *b, size_t size) { 
    if (b->info.size > size + sizeof(Block) + sizeof(Tag)) {
        Block *bolunmus_blok	= (Block *)((char *)b + size + sizeof(Block) + sizeof(Tag)); /* Bolunmus bloga isaretciyi al */
        bolunmus_blok->info.size = b->info.size - size - sizeof(Block) - sizeof(Tag);
        bolunmus_blok->info.isfree = 1;
        bolunmus_blok->next = b->next;
        bolunmus_blok->prev = b;
        /* Orjinal blok boyutunu guncelle */
        b->info.size = size;
        b->next = bolunmus_blok;
        return b;
    }
    else {
        return b;
    }
}

/* Blogu sol tarafi ile birlestirme islemi */
Block *left_coalesce(Block *b) { 
     if (prev_block_in_addr(b) != NULL && prev_block_in_addr(b)->info.isfree) {
        /* Bloklari birlestir */
        prev_block_in_addr(b)->info.size += b->info.size + sizeof(Block) + sizeof(Tag);
        prev_block_in_addr(b)->next = b->next;
        
        /* Sonraki blogun prev pointerini guncelle */
        if (b->next != NULL)
            b->next->prev = prev_block_in_addr(b);
        return prev_block_in_addr(b);
    }
    else {
        return b;
    }
}

/* Blogu sag tarafi ile birlestirme islemi */
Block *right_coalesce(Block *b) { 
    if (next_block_in_addr(b) != NULL && next_block_in_addr(b)->info.isfree) {
        /* Bloklari birlestir */
        b->info.size += next_block_in_addr(b)->info.size + sizeof(Block) + sizeof(Tag);
        b->next = next_block_in_addr(b)->next;
        
        /* Sonraki blogun prev pointerini guncelle */
        if (next_block_in_addr(b)->next != NULL)
            next_block_in_addr(b)->next->prev = b;
    }
    return b;
}

/* Free listteki bir sonraki blogu al */
Block *next_block_in_freelist(Block *b) { 
      if (b != NULL) {
        return b->next;
    } else {
        return NULL;
    }
}

/* Free listteki bir onceki blogu al */
Block *prev_block_in_freelist(Block *b) { 
     if (b != NULL) {
        return b->prev;
    } else {
        return NULL;
    }
}

/* Memory addressteki bir sonraki blogu al */
Block *next_block_in_addr(Block *b) { 
    if (b != NULL) {
        size_t blok_boyutu = b->info.size;
        return (Block *)((char *)b + blok_boyutu + sizeof(Tag) + sizeof(Block));
    } else {
        return NULL;
    }
}

/* Memory addressteki bir onceki blogu al */
Block *prev_block_in_addr(Block *b) { 
     if (b != NULL) {
        size_t blok_boyutu = prev_block_in_addr(b)->info.size;
        return (Block *)((char *)b - blok_boyutu - sizeof(Tag) - sizeof(Block));
    } else {
        return NULL;
    }
}

/* Belirli bir boyut icin gereken 16 bytelik blok sayisini byte cinsinden hesaplama islevi */
uint64_t numberof16blocks(size_t size_inbytes) { 
    return (size_inbytes + 15) / 16; 
}

/** prints meta data of the blocks
 * --------
 * size:
 * free:
 * --------
 */
void printheap() {
    Block *current = heap_start;
    printf("Blocks\n");

    /* Tum bloklari gec*/
    while (current != NULL && current<heap_end) {
        printf("Free: %d\n", current->info.isfree);
        printf("Size: %" PRIu64 "\n", current->info.size);
        printf("---------------\n");
        /* Bie sonraki bloga gec */
        current = (Block *)((char *)current + current->info.size + sizeof(Block) + sizeof(Tag));  
    }
}

/* List type i al  */
ListType getlisttype() { 
    return listtype; 
}

/* List type i  ayarla */
int setlisttype(ListType type) { 
    listtype = type;
    return 0; 
}

/* Strategy i al */
Strategy getstrategy() { 
    return strategy; 
}

/* Strategy i ayarla */
int setstrategy(Strategy strat) { 
    strategy = strat;
    return 0; 
}
