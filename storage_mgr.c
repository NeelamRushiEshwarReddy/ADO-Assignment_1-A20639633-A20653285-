#include "storage_mgr.h"
#include "dberror.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


/* Initialize the storage manager */
void 
initStorageManager(void) 
{
    // Initialize any global variables or data structures if needed
    // For this simple implementation, no initialization is required
}

/* Create a new page file with one empty page */
RC 
createPageFile(char *fileName) 
{
    FILE *file;
    char *emptyPage;
    
    if (fileName == NULL)
        THROW(RC_FILE_NOT_FOUND, "Invalid filename");
    
    // Create/open file for writing
    file = fopen(fileName, "wb");
    if (file == NULL)
        THROW(RC_WRITE_FAILED, "Could not create page file");
    
    // Allocate memory for an empty page
    emptyPage = (char *) calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL) {
        fclose(file);
        THROW(RC_WRITE_FAILED, "Memory allocation failed");
    }
    
    // Write empty page to file
    if (fwrite(emptyPage, sizeof(char), PAGE_SIZE, file) != PAGE_SIZE) {
        free(emptyPage);
        fclose(file);
        THROW(RC_WRITE_FAILED, "Could not write empty page");
    }
    
    // Clean up
    free(emptyPage);
    fclose(file);
    
    return RC_OK;
}

/* Open an existing page file */
RC 
openPageFile(char *fileName, SM_FileHandle *fHandle) 
{
    FILE *file;
    struct stat fileStat;
    
    if (fileName == NULL || fHandle == NULL)
        THROW(RC_FILE_NOT_FOUND, "Invalid parameters");
    
    // Check if file exists
    if (stat(fileName, &fileStat) != 0)
        THROW(RC_FILE_NOT_FOUND, "File does not exist");
    
    // Open file for reading and writing
    file = fopen(fileName, "rb+");
    if (file == NULL)
        THROW(RC_FILE_NOT_FOUND, "Could not open file");
    
    // Initialize file handle
    fHandle->fileName = (char *) malloc(strlen(fileName) + 1);
    if (fHandle->fileName == NULL) {
        fclose(file);
        THROW(RC_WRITE_FAILED, "Memory allocation failed");
    }
    strcpy(fHandle->fileName, fileName);
    
    // Calculate total number of pages
    fHandle->totalNumPages = (int) (fileStat.st_size / PAGE_SIZE);
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = file;  // Store FILE pointer for later use
    
    return RC_OK;
}

/* Close a page file */
RC 
closePageFile(SM_FileHandle *fHandle) 
{
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    if (fHandle->mgmtInfo == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File not open");
    
    // Close the file
    if (fclose((FILE *)fHandle->mgmtInfo) != 0)
        THROW(RC_WRITE_FAILED, "Could not close file");
    
    // Free allocated memory
    if (fHandle->fileName != NULL) {
        free(fHandle->fileName);
        fHandle->fileName = NULL;
    }
    
    // Reset handle
    fHandle->totalNumPages = 0;
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = NULL;
    
    return RC_OK;
}

/* Destroy (delete) a page file */
RC 
destroyPageFile(char *fileName) 
{
    if (fileName == NULL)
        THROW(RC_FILE_NOT_FOUND, "Invalid filename");
    
    if (unlink(fileName) != 0)
        THROW(RC_FILE_NOT_FOUND, "Could not delete file");
    
    return RC_OK;
}

/* Read a specific block/page from file */
RC 
readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    FILE *file;
    
    if (fHandle == NULL || memPage == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "Invalid parameters");
    
    if (fHandle->mgmtInfo == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File not open");
    
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
        THROW(RC_READ_NON_EXISTING_PAGE, "Page does not exist");
    
    file = (FILE *)fHandle->mgmtInfo;
    
    // Seek to the page position
    if (fseek(file, pageNum * PAGE_SIZE, SEEK_SET) != 0)
        THROW(RC_READ_NON_EXISTING_PAGE, "Could not seek to page");
    
    // Read the page
    if (fread(memPage, sizeof(char), PAGE_SIZE, file) != PAGE_SIZE)
        THROW(RC_READ_NON_EXISTING_PAGE, "Could not read page");
    
    // Update current page position
    fHandle->curPagePos = pageNum;
    
    return RC_OK;
}

/* Get current page position */
int 
getBlockPos(SM_FileHandle *fHandle) 
{
    if (fHandle == NULL)
        return -1;
    
    return fHandle->curPagePos;
}

/* Read the first page */
RC 
readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    return readBlock(0, fHandle, memPage);
}

/* Read the previous page */
RC 
readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    if (fHandle->curPagePos <= 0)
        THROW(RC_READ_NON_EXISTING_PAGE, "No previous page");
    
    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

/* Read the current page */
RC 
readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

/* Read the next page */
RC 
readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    if (fHandle->curPagePos >= fHandle->totalNumPages - 1)
        THROW(RC_READ_NON_EXISTING_PAGE, "No next page");
    
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

/* Read the last page */
RC 
readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    if (fHandle->totalNumPages == 0)
        THROW(RC_READ_NON_EXISTING_PAGE, "No pages in file");
    
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

/* Write a page to a specific position */
RC 
writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    FILE *file;
    
    if (fHandle == NULL || memPage == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "Invalid parameters");
    
    if (fHandle->mgmtInfo == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File not open");
    
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
        THROW(RC_WRITE_FAILED, "Invalid page number");
    
    file = (FILE *)fHandle->mgmtInfo;
    
    // Seek to the page position
    if (fseek(file, pageNum * PAGE_SIZE, SEEK_SET) != 0)
        THROW(RC_WRITE_FAILED, "Could not seek to page");
    
    // Write the page
    if (fwrite(memPage, sizeof(char), PAGE_SIZE, file) != PAGE_SIZE)
        THROW(RC_WRITE_FAILED, "Could not write page");
    
    // Flush to ensure data is written
    fflush(file);
    
    // Update current page position
    fHandle->curPagePos = pageNum;
    
    return RC_OK;
}

/* Write to the current page */
RC 
writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

/* Append an empty block to the file */
RC 
appendEmptyBlock(SM_FileHandle *fHandle) 
{
    FILE *file;
    char *emptyPage;
    
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    if (fHandle->mgmtInfo == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File not open");
    
    file = (FILE *)fHandle->mgmtInfo;
    
    // Allocate memory for an empty page
    emptyPage = (char *) calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL)
        THROW(RC_WRITE_FAILED, "Memory allocation failed");
    
    // Seek to end of file
    if (fseek(file, 0, SEEK_END) != 0) {
        free(emptyPage);
        THROW(RC_WRITE_FAILED, "Could not seek to end of file");
    }
    
    // Write empty page
    if (fwrite(emptyPage, sizeof(char), PAGE_SIZE, file) != PAGE_SIZE) {
        free(emptyPage);
        THROW(RC_WRITE_FAILED, "Could not append empty page");
    }
    
    // Flush to ensure data is written
    fflush(file);
    
    // Update total number of pages
    fHandle->totalNumPages++;
    
    // Clean up
    free(emptyPage);
    
    return RC_OK;
}

/* Ensure the file has at least numberOfPages pages */
RC 
ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) 
{
    RC result;
    
    if (fHandle == NULL)
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    
    if (numberOfPages <= 0)
        THROW(RC_WRITE_FAILED, "Invalid number of pages");
    
    // Append empty blocks until we have enough pages
    while (fHandle->totalNumPages < numberOfPages) {
        result = appendEmptyBlock(fHandle);
        if (result != RC_OK)
            return result;
    }
    
    return RC_OK;
}