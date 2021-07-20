/*
 * VMS - Versatile Modulation System
 *
 * Copyright (c) 2020 Thorben Zethoff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "NoteManager.h"
#include <stdlib.h>


uint8_t NoteManager_currNotes = 0;

NoteListElement * NoteManager_head = 0;

void NoteManager_init(){
    NoteManager_head = malloc(sizeof(NoteListElement));
    NoteManager_head->next = NoteManager_head;
    NoteManager_head->prev = NoteManager_head;
    NoteManager_head->data.note = 0;
}

void NoteManager_addNote(uint8_t note, uint8_t velocity){
    //find first note that is higher than the one we are playing
    NoteListElement * currElement = NoteManager_head->next;
    while(currElement != NoteManager_head) {
        if(currElement->data.note == note) return; //our note is already in the list -> don't add anything
        if(currElement->data.note > note) break;
        currElement = currElement->next;
    }
    
    //allocate memory for the new note
    NoteListElement * newNote = malloc(sizeof(NoteListElement));
    newNote->data.note = note;
    newNote->data.velocity = velocity;
    
    
    //add out note behind it
    newNote->prev = currElement->prev;
    newNote->next = currElement;
    currElement->prev->next = newNote;
    currElement->prev = newNote;
}

void NoteManager_removeNote(uint8_t note){
    //find the note to delete
    NoteListElement * currElement = NoteManager_head->next;
    while(currElement != NoteManager_head) {
        if(currElement->data.note == note) break;
        currElement = currElement->next;
    }
    
    if(currElement == NoteManager_head) return;
    
    //re-assign surrounding list elements
    currElement->prev->next = currElement->next;
    currElement->next->prev = currElement->prev;
    
    //free resources
    free(currElement);
}

void NoteManager_clearList(){
    NoteListElement * currElement = NoteManager_head->next;
    while(currElement != NoteManager_head) {
        NoteListElement * nextElement = currElement->next;
        free(currElement);
        currElement = nextElement;
    }
    NoteManager_head->next = NoteManager_head;
    NoteManager_head->prev = NoteManager_head;
}

uint8_t NoteManager_dumpNotes(uint8_t * dst){
    uint8_t currNoteCount = 0;
    
    NoteListElement * currElement = NoteManager_head->next;
    while(currElement != NoteManager_head) {
        dst[currNoteCount * 2] = currElement->data.note;
        dst[currNoteCount * 2 + 1] = currElement->data.velocity;
        currNoteCount ++;
        currElement = currElement->next;
    }
    
    return currNoteCount;
}