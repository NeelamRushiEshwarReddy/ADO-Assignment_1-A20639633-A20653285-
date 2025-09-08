#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_additional.bin"

/* prototypes for test functions */
static void testMultiplePages(void);
static void testNavigationMethods(void);
static void testEnsureCapacity(void);
static void testErrorConditions(void);

/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();

  testMultiplePages();
  testNavigationMethods();
  testEnsureCapacity();
  testErrorConditions();

  return 0;
}

/* Test multiple page operations */
void
testMultiplePages(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test multiple pages";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("Testing multiple pages\n");
  
  // Add a few more pages
  TEST_CHECK(appendEmptyBlock(&fh));
  TEST_CHECK(appendEmptyBlock(&fh));
  ASSERT_TRUE((fh.totalNumPages == 3), "should have 3 pages after appending 2");

  // Write different data to each page
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = 'A' + (i % 26);
  TEST_CHECK(writeBlock(0, &fh, ph));

  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = 'a' + (i % 26);
  TEST_CHECK(writeBlock(1, &fh, ph));

  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = '0' + (i % 10);
  TEST_CHECK(writeBlock(2, &fh, ph));

  // Read back and verify each page
  TEST_CHECK(readBlock(0, &fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 'A' + (i % 26)), "first page should contain A-Z pattern");

  TEST_CHECK(readBlock(1, &fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 'a' + (i % 26)), "second page should contain a-z pattern");

  TEST_CHECK(readBlock(2, &fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == '0' + (i % 10)), "third page should contain 0-9 pattern");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  free(ph);
  
  TEST_DONE();
}

/* Test navigation methods */
void
testNavigationMethods(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test navigation methods";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a file with multiple pages
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  TEST_CHECK(appendEmptyBlock(&fh));
  TEST_CHECK(appendEmptyBlock(&fh));
  printf("Testing navigation methods\n");

  // Write unique data to each page for identification
  for (i = 0; i < 3; i++) {
    memset(ph, 'A' + i, PAGE_SIZE);
    TEST_CHECK(writeBlock(i, &fh, ph));
  }

  // Test readFirstBlock
  TEST_CHECK(readFirstBlock(&fh, ph));
  ASSERT_TRUE((fh.curPagePos == 0), "current page should be 0 after readFirstBlock");
  ASSERT_TRUE((ph[0] == 'A'), "should read first page content");

  // Test readLastBlock  
  TEST_CHECK(readLastBlock(&fh, ph));
  ASSERT_TRUE((fh.curPagePos == 2), "current page should be 2 after readLastBlock");
  ASSERT_TRUE((ph[0] == 'C'), "should read last page content");

  // Test readPreviousBlock
  TEST_CHECK(readPreviousBlock(&fh, ph));
  ASSERT_TRUE((fh.curPagePos == 1), "current page should be 1 after readPreviousBlock");
  ASSERT_TRUE((ph[0] == 'B'), "should read previous page content");

  // Test readNextBlock
  TEST_CHECK(readNextBlock(&fh, ph));
  ASSERT_TRUE((fh.curPagePos == 2), "current page should be 2 after readNextBlock");
  ASSERT_TRUE((ph[0] == 'C'), "should read next page content");

  // Test readCurrentBlock
  TEST_CHECK(readCurrentBlock(&fh, ph));
  ASSERT_TRUE((fh.curPagePos == 2), "current page should remain 2 after readCurrentBlock");
  ASSERT_TRUE((ph[0] == 'C'), "should read current page content");

  // Test getBlockPos
  int pos = getBlockPos(&fh);
  ASSERT_TRUE((pos == 2), "getBlockPos should return current position");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  free(ph);
  
  TEST_DONE();
}

/* Test ensureCapacity */
void
testEnsureCapacity(void)
{
  SM_FileHandle fh;

  testName = "test ensure capacity";

  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  printf("Testing ensureCapacity\n");

  ASSERT_TRUE((fh.totalNumPages == 1), "should start with 1 page");

  // Ensure capacity for 5 pages
  TEST_CHECK(ensureCapacity(5, &fh));
  ASSERT_TRUE((fh.totalNumPages == 5), "should have 5 pages after ensureCapacity");

  // Ensure capacity for fewer pages (should not change)
  TEST_CHECK(ensureCapacity(3, &fh));
  ASSERT_TRUE((fh.totalNumPages == 5), "should still have 5 pages");

  // Ensure capacity for more pages
  TEST_CHECK(ensureCapacity(8, &fh));
  ASSERT_TRUE((fh.totalNumPages == 8), "should have 8 pages after second ensureCapacity");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  
  TEST_DONE();
}

/* Test error conditions */
void
testErrorConditions(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;

  testName = "test error conditions";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  printf("Testing error conditions\n");

  // Test reading non-existing page
  ASSERT_ERROR(readBlock(5, &fh, ph), "reading non-existing page should return error");
  ASSERT_ERROR(readBlock(-1, &fh, ph), "reading negative page should return error");

  // Test writing to non-existing page
  ASSERT_ERROR(writeBlock(5, &fh, ph), "writing to non-existing page should return error");
  ASSERT_ERROR(writeBlock(-1, &fh, ph), "writing to negative page should return error");

  // Position at first page
  TEST_CHECK(readFirstBlock(&fh, ph));
  
  // Test reading previous when at first page
  ASSERT_ERROR(readPreviousBlock(&fh, ph), "reading previous at first page should return error");

  // Position at last page
  TEST_CHECK(readLastBlock(&fh, ph));
  
  // Test reading next when at last page
  ASSERT_ERROR(readNextBlock(&fh, ph), "reading next at last page should return error");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  free(ph);
  
  TEST_DONE();
}