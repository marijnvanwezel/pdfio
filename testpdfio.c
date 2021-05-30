//
// Test program for pdfio.
//
// Copyright © 2021 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

//
// Include necessary headers...
//

#include "pdfio-private.h"
#include "pdfio-content.h"


//
// Local functions...
//

static int	do_test_file(const char *filename);
static int	do_unit_tests(void);
static bool	error_cb(pdfio_file_t *pdf, const char *message, bool *error);
static ssize_t	token_consume_cb(const char **s, size_t bytes);
static ssize_t	token_peek_cb(const char **s, char *buffer, size_t bytes);
static int	write_page(pdfio_file_t *pdf, int number, pdfio_obj_t *image);


//
// 'main()' - Main entry for test program.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  if (argc > 1)
  {
    int	i;				// Looping var

    for (i = 1; i < argc; i ++)
    {
      if (do_test_file(argv[i]))
        return (1);
    }
  }
  else
  {
    do_unit_tests();
  }

  return (0);
}


//
// 'do_test_file()' - Try loading a PDF file and listing pages and objects.
//

static int				// O - Exit status
do_test_file(const char *filename)	// I - PDF filename
{
  bool		error = false;		// Have we shown an error yet?
  pdfio_file_t	*pdf;			// PDF file
  size_t	n,			// Object/page index
		num_objs,		// Number of objects
		num_pages;		// Number of pages
  pdfio_obj_t	*obj;			// Object
  pdfio_dict_t	*dict;			// Object dictionary
  const char	*type;			// Object type


  // Try opening the file...
  printf("pdfioFileOpen(\"%s\", ...): ", filename);
  if ((pdf = pdfioFileOpen(filename, (pdfio_error_cb_t)error_cb, &error)) != NULL)
  {
    puts("PASS");

    // Show basic stats...
    num_objs  = pdfioFileGetNumObjects(pdf);
    num_pages = pdfioFileGetNumPages(pdf);

    printf("    PDF %s, %d pages, %d objects.\n", pdfioFileGetVersion(pdf), (int)num_pages, (int)num_objs);

    // Show a summary of each page...
    for (n = 0; n < num_pages; n ++)
    {
      if ((obj = pdfioFileGetPage(pdf, n)) == NULL)
      {
	printf("%s: Unable to get page #%d.\n", filename, (int)n + 1);
      }
      else
      {
	pdfio_rect_t media_box;	// MediaBox value

	memset(&media_box, 0, sizeof(media_box));
	dict = pdfioObjGetDict(obj);

	if (!pdfioDictGetRect(dict, "MediaBox", &media_box))
	{
	  if ((obj = pdfioDictGetObject(dict, "Parent")) != NULL)
	  {
	    dict = pdfioObjGetDict(obj);
	    pdfioDictGetRect(dict, "MediaBox", &media_box);
	  }
	}

	printf("    Page #%d is %gx%g.\n", (int)n + 1, media_box.x2, media_box.y2);
      }
    }

    // Show the associated value with each object...
    for (n = 0; n < num_objs; n ++)
    {
      if ((obj = pdfioFileGetObject(pdf, n)) == NULL)
      {
	printf("    Unable to get object #%d.\n", (int)n);
      }
      else
      {
	dict = pdfioObjGetDict(obj);

	printf("    %u %u obj dict=%p(%lu pairs)\n", (unsigned)pdfioObjGetNumber(obj), (unsigned)pdfioObjGetGeneration(obj), dict, dict ? (unsigned long)dict->num_pairs : 0UL);
	fputs("        ", stdout);
	_pdfioValueDebug(&obj->value, stdout);
	putchar('\n');
      }
    }

    // Close the file and return success...
    pdfioFileClose(pdf);
    return (0);
  }
  else
  {
    // Error message will already be displayed so just indicate failure...
    return (1);
  }
}


//
// 'do_unit_tests()' - Do unit tests.
//

static int				// O - Exit status
do_unit_tests(void)
{
  int			i;		// Looping var
  char			filename[256];	// PDF filename
  pdfio_file_t		*pdf,		// Test PDF file
			*outpdf;	// Output PDF file
  bool			error = false;	// Error callback data
  _pdfio_token_t	tb;		// Token buffer
  const char		*s;		// String buffer
  _pdfio_value_t	value;		// Value
  pdfio_obj_t		*color_jpg,	// color.jpg image
			*gray_jpg,	// gray.jpg image
			*page;		// Page from test PDF file
  static const char	*complex_dict =	// Complex dictionary value
    "<</Annots 5457 0 R/Contents 5469 0 R/CropBox[0 0 595.4 842]/Group 725 0 R"
    "/MediaBox[0 0 595.4 842]/Parent 23513 0 R/Resources<</ColorSpace<<"
    "/CS0 21381 0 R/CS1 21393 0 R>>/ExtGState<</GS0 21420 0 R>>/Font<<"
    "/TT0 21384 0 R/TT1 21390 0 R/TT2 21423 0 R/TT3 21403 0 R/TT4 21397 0 R>>"
    "/ProcSet[/PDF/Text/ImageC]/Properties<</MC0 5472 0 R/MC1 5473 0 R>>"
    "/XObject<</E3Dp0QGN3h9EZL2X 23690 0 R/E6DU0TGl3s9NZT2C 23691 0 R"
    "/ENDB06GH3u9tZT2N 21391 0 R/ENDD0NGM339cZe2F 23692 0 R"
    "/ENDK00GK3c9DZN2n 23693 0 R/EPDB0NGN3Q9GZP2t 23695 0 R"
    "/EpDA0kG03o9rZX21 23696 0 R/Im0 5475 0 R>>>>/Rotate 0/StructParents 2105"
    "/Tabs/S/Type/Page>>";


  setbuf(stdout, NULL);

  // First open the test PDF file...
  fputs("pdfioFileOpen(\"testfiles/testpdfio.pdf\"): ", stdout);
  if ((pdf = pdfioFileOpen("testfiles/testpdfio.pdf", (pdfio_error_cb_t)error_cb, &error)) != NULL)
    puts("PASS");
  else
    return (1);

  // TODO: Test for known values in this test file.

  // Test the value parsers for edge cases...
  fputs("_pdfioValueRead(complex_dict): ", stdout);
  s = complex_dict;
  _pdfioTokenInit(&tb, pdf, (_pdfio_tconsume_cb_t)token_consume_cb, (_pdfio_tpeek_cb_t)token_peek_cb, &s);
  if (_pdfioValueRead(pdf, &tb, &value))
  {
    // TODO: Check value...
    puts("PASS");
  }
  else
    return (1);

  // Create a new PDF file...
  fputs("pdfioFileCreate(\"testpdfio-out.pdf\", ...): ", stdout);
  if ((outpdf = pdfioFileCreate("testpdfio-out.pdf", NULL, NULL, NULL, (pdfio_error_cb_t)error_cb, &error)) != NULL)
    puts("PASS");
  else
    return (1);

  // Create some image objects...
  fputs("pdfioFileCreateImageObject(\"testfiles/color.jpg\"): ", stdout);
  if ((color_jpg = pdfioFileCreateImageObject(outpdf, "testfiles/color.jpg", true)) != NULL)
    puts("PASS");
  else
    return (1);

  fputs("pdfioFileCreateImageObject(\"testfiles/gray.jpg\"): ", stdout);
  if ((gray_jpg = pdfioFileCreateImageObject(outpdf, "testfiles/gray.jpg", true)) != NULL)
    puts("PASS");
  else
    return (1);

  // Copy the first page from the test PDF file...
  fputs("pdfioFileGetPage(0): ", stdout);
  if ((page = pdfioFileGetPage(pdf, 0)) != NULL)
    puts("PASS");
  else
    return (1);

  fputs("pdfioPageCopy(first page): ", stdout);
  if (pdfioPageCopy(outpdf, page))
    puts("PASS");
  else
    return (1);

  // Write a few pages...
  for (i = 1; i < 16; i ++)
  {
    if (write_page(outpdf, i, (i & 1) ? color_jpg : gray_jpg))
      return (1);
  }

  // Copy the third page from the test PDF file...
  fputs("pdfioFileGetPage(2): ", stdout);
  if ((page = pdfioFileGetPage(pdf, 2)) != NULL)
    puts("PASS");
  else
    return (1);

  fputs("pdfioPageCopy(third page): ", stdout);
  if (pdfioPageCopy(outpdf, page))
    puts("PASS");
  else
    return (1);

  // Close the test PDF file...
  fputs("pdfioFileClose(\"testfiles/testpdfio.pdf\": ", stdout);
  if (pdfioFileClose(pdf))
    puts("PASS");
  else
    return (1);

  // Close the new PDF file...
  fputs("pdfioFileClose(\"testpdfio-out.pdf\": ", stdout);
  if (pdfioFileClose(outpdf))
    puts("PASS");
  else
    return (1);

  return (0);
}


//
// 'error_cb()' - Display an error message during a unit test.
//

static bool				// O  - `true` to stop, `false` to continue
error_cb(pdfio_file_t *pdf,		// I  - PDF file
         const char   *message,		// I  - Error message
	 bool         *error)		// IO - Have we displayed an error? 
{
  (void)pdf;

  if (!*error)
  {
    // First error, so show a "FAIL" indicator
    *error = true;

    puts("FAIL");
  }

  // Indent error messages...
  printf("    %s\n", message);

  // Continue to catch more errors...
  return (false);
}


//
// 'token_consume_cb()' - Consume bytes from a test string.
//

static ssize_t				// O  - Number of bytes consumed
token_consume_cb(const char **s,	// IO - Test string
                 size_t     bytes)	// I  - Bytes to consume
{
  size_t	len = strlen(*s);	// Number of bytes remaining


  // "Consume" bytes by incrementing the string pointer, limiting to the
  // remaining length...
  if (bytes > len)
    bytes = len;

  *s += bytes;

  return ((ssize_t)bytes);
}


//
// 'token_peek_cb()' - Peek bytes from a test string.
//

static ssize_t				// O  - Number of bytes peeked
token_peek_cb(const char **s,		// IO - Test string
              char       *buffer,	// I  - Buffer
              size_t     bytes)		// I  - Bytes to peek
{
  size_t	len = strlen(*s);	// Number of bytes remaining


  // Copy bytes from the test string as possible...
  if (bytes > len)
    bytes = len;

  if (bytes > 0)
    memcpy(buffer, *s, bytes);

  return ((ssize_t)bytes);
}


//
// 'write_page()' - Write a page to a PDF file.
//

static int				// O - 1 on failure, 0 on success
write_page(pdfio_file_t *pdf,		// I - PDF file
           int          number,		// I - Page number
           pdfio_obj_t  *image)		// I - Image to draw
{
  // TODO: Add font object support...
  pdfio_dict_t		*dict;		// Page dictionary
  pdfio_stream_t	*st;		// Page contents stream
  double		width,		// Width of image
			height;		// Height of image
  double		swidth,		// Scaled width
			sheight,	// Scaled height
			tx,		// X offset
			ty;		// Y offset


  fputs("pdfioDictCreate: ", stdout);
  if ((dict = pdfioDictCreate(pdf)) != NULL)
    puts("PASS");
  else
    return (1);

  fputs("pdfioPageDictAddImage: ", stdout);
  if (pdfioPageDictAddImage(dict, "IM1", image))
    puts("PASS");
  else
    return (1);

  printf("pdfioFileCreatePage(%d): ", number);

  if ((st = pdfioFileCreatePage(pdf, dict)) != NULL)
    puts("PASS");
  else
    return (1);

  fputs("pdfioStreamPuts(...): ", stdout);
  if (pdfioStreamPuts(st,
                      "1 0 0 RG 0 g 5 w\n"
                      "18 18 559 760 re 72 72 451 648 re B*\n"))
    puts("PASS");
  else
    return (1);

  fputs("pdfioContentSave(): ", stdout);
  if (pdfioContentSave(st))
    puts("PASS");
  else
    return (1);

  fputs("pdfioImageGetWidth(): ", stdout);
  if ((width = pdfioImageGetWidth(image)) > 0.0)
    puts("PASS");
  else
    return (1);

  fputs("pdfioImageGetHeight(): ", stdout);
  if ((height = pdfioImageGetHeight(image)) > 0.0)
    puts("PASS");
  else
    return (1);

  swidth  = 400.0;
  sheight = swidth * height / width;
  if (sheight > 600.0)
  {
    sheight = 600.0;
    swidth  = sheight * width / height;
  }

  tx = 0.5 * (595.28 - swidth);
  ty = 0.5 * (792 - sheight);

  printf("pdfioContentDrawImage(\"IM1\", x=%g, y=%g, w=%g, h=%g): ", tx, ty, swidth, sheight);
  if (pdfioContentDrawImage(st, "IM1", tx, ty, swidth, sheight))
    puts("PASS");
  else
    return (1);

  fputs("pdfioContentRestore(): ", stdout);
  if (pdfioContentRestore(st))
    puts("PASS");
  else
    return (1);

  fputs("pdfioStreamClose: ", stdout);
  if (pdfioStreamClose(st))
    puts("PASS");
  else
    return (1);

  return (0);
}
