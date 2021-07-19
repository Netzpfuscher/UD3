
#if !defined(NoteManager_H)
#define NoteManager_H
    
    #if PIC32
    #include <xc.h>
    #endif
    #include <stdint.h>

    typedef struct{
        uint8_t note;
        uint8_t velocity;
    } Note;

    typedef struct DLLElement{
        struct DLLElement *next;
        struct DLLElement *prev;
        Note data;
    } NoteListElement;    

    void NoteManager_init();

    void NoteManager_addNote(uint8_t note, uint8_t velocity);

    void NoteManager_removeNote(uint8_t note);

    uint8_t NoteManager_dumpNotes(uint8_t * dst);

    void NoteManager_clearList();


#endif