/**
 * @file NoteManager.h
 * @brief MIDI note list manager for polyphonic voice allocation
 *
 * Maintains a sorted doubly-linked list of active MIDI notes for voice management.
 * Used by the MIDI processor to track which notes are currently playing.
 */

#if !defined(NoteManager_H)
#define NoteManager_H
    
    #if PIC32
    #include <xc.h>
    #endif
    #include <stdint.h>

    /**
     * @brief MIDI note data structure
     */
    typedef struct{
        uint8_t note;       //!< MIDI note number (0-127)
        uint8_t velocity;   //!< Note velocity (0-127)
    } Note;

    /**
     * @brief Doubly-linked list element for note storage
     *
     * Notes are stored in ascending order by note number.
     */
    typedef struct DLLElement{
        struct DLLElement *next;  //!< Next element in list
        struct DLLElement *prev;  //!< Previous element in list
        Note data;                //!< Note data
    } NoteListElement;    

    /**
     * @brief Initialize the note manager
     *
     * Allocates and initializes the circular doubly-linked list head.
     * Must be called before any other NoteManager functions.
     */
    void NoteManager_init();

    /**
     * @brief Add a note to the active note list
     *
     * Inserts the note in sorted order (ascending by note number).
     * If the note is already in the list, it is not added again.
     *
     * @param note MIDI note number (0-127)
     * @param velocity Note velocity (0-127)
     */
    void NoteManager_addNote(uint8_t note, uint8_t velocity);

    /**
     * @brief Remove a note from the active note list
     *
     * Searches for the note and removes it if found. Does nothing if the note
     * is not in the list.
     *
     * @param note MIDI note number to remove (0-127)
     */
    void NoteManager_removeNote(uint8_t note);

    /**
     * @brief Dump all active notes to a buffer
     *
     * Copies note/velocity pairs to the destination buffer in sorted order.
     * Each note takes 2 bytes: [note, velocity, note, velocity, ...]
     *
     * @param dst Destination buffer (must be at least 2*count bytes)
     * @return Number of notes copied
     */
    uint8_t NoteManager_dumpNotes(uint8_t * dst);

    /**
     * @brief Clear all notes from the list
     *
     * Removes and frees all note elements, leaving only the list head.
     * Useful for handling MIDI panic or all-notes-off messages.
     */
    void NoteManager_clearList();


#endif