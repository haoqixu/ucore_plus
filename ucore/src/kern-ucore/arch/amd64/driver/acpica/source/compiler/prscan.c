/******************************************************************************
 *
 * Module Name: prscan - Preprocessor start-up and file scan module
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2012, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#define _DECLARE_PR_GLOBALS

#include "aslcompiler.h"
#include "dtcompiler.h"

/*
 * TBDs:
 *
 * No nested macros, maybe never
 * Implement ASL "Include" as well as "#include" here?
 */
#define _COMPONENT          ASL_PREPROCESSOR
        ACPI_MODULE_NAME    ("prscan")


/* Local prototypes */

static void
PrPreprocessInputFile (
    void);

static void
PrDoDirective (
    char                    *DirectiveToken,
    char                    **Next,
    BOOLEAN                 *IgnoringThisCodeBlock);

static int
PrMatchDirective (
    char                    *Directive);

/*
 * Supported preprocessor directives
 */
static const PR_DIRECTIVE_INFO      Gbl_DirectiveInfo[] =
{
    {"define",  1},
    {"elif",    0}, /* Converted to #else..#if internally */
    {"else",    0},
    {"endif",   0},
    {"error",   1},
    {"if",      1},
    {"ifdef",   1},
    {"ifndef",  1},
    {"include", 0}, /* Argument is not standard format, so 0 */
    {"line",    1},
    {"pragma",  1},
    {"undef",   1},
    {"warning", 1},
    {NULL,      0}
};

enum Gbl_DirectiveIndexes
{
    PR_DIRECTIVE_DEFINE = 0,
    PR_DIRECTIVE_ELIF,
    PR_DIRECTIVE_ELSE,
    PR_DIRECTIVE_ENDIF,
    PR_DIRECTIVE_ERROR,
    PR_DIRECTIVE_IF,
    PR_DIRECTIVE_IFDEF,
    PR_DIRECTIVE_IFNDEF,
    PR_DIRECTIVE_INCLUDE,
    PR_DIRECTIVE_LINE,
    PR_DIRECTIVE_PRAGMA,
    PR_DIRECTIVE_UNDEF,
    PR_DIRECTIVE_WARNING,
};

#define ASL_DIRECTIVE_NOT_FOUND     -1


/*******************************************************************************
 *
 * FUNCTION:    PrInitializePreprocessor
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Startup initialization for the Preprocessor.
 *
 ******************************************************************************/

void
PrInitializePreprocessor (
    void)
{
    /* Init globals and the list of #defines */

    PrInitializeGlobals ();
    Gbl_DefineList = NULL;
}


/*******************************************************************************
 *
 * FUNCTION:    PrInitializeGlobals
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize globals for the Preprocessor. Used for startuup
 *              initialization and re-initialization between compiles during
 *              a multiple source file compile.
 *
 ******************************************************************************/

void
PrInitializeGlobals (
    void)
{
    /* Init globals */

    Gbl_IfDepth = 0;
    Gbl_InputFileList = NULL;
    Gbl_CurrentLineNumber = 0;
    Gbl_PreprocessorLineNumber = 1;
    Gbl_PreprocessorError = FALSE;
}


/*******************************************************************************
 *
 * FUNCTION:    PrTerminatePreprocessor
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Termination of the preprocessor. Delete lists. Keep any
 *              defines that were specified on the command line, in order to
 *              support multiple compiles with a single compiler invocation.
 *
 ******************************************************************************/

void
PrTerminatePreprocessor (
    void)
{
    PR_DEFINE_INFO          *DefineInfo;


    /*
     * The persistent defines (created on the command line) are always at the
     * end of the list. We save them.
     */
    while ((Gbl_DefineList) && (!Gbl_DefineList->Persist))
    {
        DefineInfo = Gbl_DefineList;
        Gbl_DefineList = DefineInfo->Next;

        ACPI_FREE (DefineInfo->Replacement);
        ACPI_FREE (DefineInfo->Identifier);
        ACPI_FREE (DefineInfo);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    PrDoPreprocess
 *
 * PARAMETERS:  None
 *
 * RETURN:      Error Status. TRUE if error, FALSE if OK.
 *
 * DESCRIPTION: Main entry point for the iASL Preprocessor. Input file must
 *              be already open. Handles multiple input files via the
 *              #include directive.
 *
 ******************************************************************************/

BOOLEAN
PrDoPreprocess (
    void)
{
    BOOLEAN                 MoreInputFiles;


    DbgPrint (ASL_DEBUG_OUTPUT, "Starting preprocessing phase\n\n");


    FlSeekFile (ASL_FILE_INPUT, 0);
    PrDumpPredefinedNames ();

    /* Main preprocessor loop, handles include files */

    do
    {
        PrPreprocessInputFile ();
        MoreInputFiles = PrPopInputFileStack ();

    } while (MoreInputFiles);


    /*
     * TBD: is this necessary? (Do we abort on any preprocessing errors?)
     */
    if (Gbl_PreprocessorError)
    {
        /* TBD: can't use source_output file for preprocessor error reporting */

        Gbl_Files[ASL_FILE_SOURCE_OUTPUT].Handle = NULL;
        PrTerminatePreprocessor ();
        return (TRUE);
    }

    /* Point compiler input to the new preprocessor file (.i) */

    FlCloseFile (ASL_FILE_INPUT);
    Gbl_Files[ASL_FILE_INPUT].Handle = Gbl_Files[ASL_FILE_PREPROCESSOR].Handle;
    AslCompilerin = Gbl_Files[ASL_FILE_INPUT].Handle;

    /* Reset globals to allow compiler to run */

    FlSeekFile (ASL_FILE_INPUT, 0);
    Gbl_CurrentLineNumber = 1;

    DbgPrint (ASL_DEBUG_OUTPUT, "Preprocessing phase complete \n\n");
    return (FALSE);
}


/*******************************************************************************
 *
 * FUNCTION:    PrPreprocessInputFile
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Preprocess one entire file, line-by-line.
 *
 * Input:  Raw user ASL from ASL_FILE_INPUT
 * Output: Preprocessed file written to ASL_FILE_PREPROCESSOR
 *
 ******************************************************************************/

static void
PrPreprocessInputFile (
    void)
{
    UINT32                  Offset;
    char                    *Token;
    char                    *ReplaceString;
    PR_DEFINE_INFO          *DefineInfo;
    ACPI_SIZE               TokenOffset;
    BOOLEAN                 IgnoringThisCodeBlock = FALSE;
    char                    *Next;
    int                     OffsetAdjust;


    /* Scan line-by-line. Comments and blank lines are skipped by this function */

    while ((Offset = DtGetNextLine (Gbl_Files[ASL_FILE_INPUT].Handle)) != ASL_EOF)
    {
        /* Need a copy of the input line for strok() */

        strcpy (Gbl_MainTokenBuffer, Gbl_CurrentLineBuffer);
        Token = PrGetNextToken (Gbl_MainTokenBuffer, PR_TOKEN_SEPARATORS, &Next);
        OffsetAdjust = 0;

        /* All preprocessor directives must begin with '#' */

        if (Token && (*Token == '#'))
        {
            if (strlen (Token) == 1)
            {
                Token = PrGetNextToken (NULL, PR_TOKEN_SEPARATORS, &Next);
            }
            else
            {
                Token++;    /* Skip leading # */
            }

            /* Execute the directive, do not write line to output file */

            PrDoDirective (Token, &Next, &IgnoringThisCodeBlock);
            continue;
        }

        /*
         * If we are currently within the part of an IF/ELSE block that is
         * FALSE, ignore the line and do not write it to the output file.
         * This continues until an #else or #endif is encountered.
         */
        if (IgnoringThisCodeBlock == TRUE)
        {
            continue;
        }

        /* Match and replace all #defined names within this source line */

        while (Token)
        {
            DefineInfo = PrMatchDefine (Token);
            if (DefineInfo)
            {
                if (DefineInfo->Body)
                {
                    /* This is a macro */

                    DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
                        "Matched Macro: %s->%s\n",
                        Gbl_CurrentLineNumber, DefineInfo->Identifier,
                        DefineInfo->Replacement);

                    PrDoMacroInvocation (Gbl_MainTokenBuffer, Token,
                        DefineInfo, &Next);
                }
                else
                {
                    ReplaceString = DefineInfo->Replacement;

                    /* Replace the name in the original line buffer */

                    TokenOffset = Token - Gbl_MainTokenBuffer + OffsetAdjust;
                    PrReplaceData (
                        &Gbl_CurrentLineBuffer[TokenOffset], strlen (Token),
                        ReplaceString, strlen (ReplaceString));

                    /* Adjust for length difference between old and new name length */

                    OffsetAdjust += strlen (ReplaceString) - strlen (Token);

                    DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
                        "Matched #define: %s->%s\n",
                        Gbl_CurrentLineNumber, Token,
                        *ReplaceString ? ReplaceString : "(NULL STRING)");
                }
            }

            Token = PrGetNextToken (NULL, PR_TOKEN_SEPARATORS, &Next);
        }

#if 0
/* Line prefix */
        FlPrintFile (ASL_FILE_PREPROCESSOR, "/* %14s  %.5u  i:%.5u */ ",
            Gbl_Files[ASL_FILE_INPUT].Filename,
            Gbl_CurrentLineNumber, Gbl_PreprocessorLineNumber);
#endif

        /*
         * Emit a #line directive if necessary, to keep the line numbers in
         * the (.i) file synchronized with the original source code file, so
         * that the correct line number appears in any error messages
         * generated by the actual compiler.
         */
        if (Gbl_CurrentLineNumber > (Gbl_PreviousLineNumber + 1))
        {
            FlPrintFile (ASL_FILE_PREPROCESSOR, "#line %u\n",
                Gbl_CurrentLineNumber);
        }

        Gbl_PreviousLineNumber = Gbl_CurrentLineNumber;
        Gbl_PreprocessorLineNumber++;

        /*
         * Now we can write the possibly modified source line to the
         * preprocessor (.i) file
         */
        FlWriteFile (ASL_FILE_PREPROCESSOR, Gbl_CurrentLineBuffer,
            strlen (Gbl_CurrentLineBuffer));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    PrDoDirective
 *
 * PARAMETERS:  Directive               - Pointer to directive name token
 *              Next                    - "Next" buffer from GetNextToken
 *              IgnoringThisCodeBlock   - Where the "ignore code" flag is
 *                                        returned.
 *
 * RETURN:      IgnoringThisCodeBlock: Set to TRUE if we are skipping the FALSE
 *              part of an #if or #else block. Set to FALSE when the
 *              corresponding #else or #endif is encountered.
 *
 * DESCRIPTION: Main processing for all preprocessor directives
 *
 ******************************************************************************/

static void
PrDoDirective (
    char                    *DirectiveToken,
    char                    **Next,
    BOOLEAN                 *IgnoringThisCodeBlock)
{
    char                    *Token = Gbl_MainTokenBuffer;
    char                    *Token2;
    char                    *End;
    UINT64                  Value;
    ACPI_SIZE               TokenOffset;
    int                     Directive;
    ACPI_STATUS             Status;


    if (!DirectiveToken)
    {
        goto SyntaxError;
    }

    Directive = PrMatchDirective (DirectiveToken);
    if (Directive == ASL_DIRECTIVE_NOT_FOUND)
    {
        PrError (ASL_ERROR, ASL_MSG_UNKNOWN_DIRECTIVE,
            THIS_TOKEN_OFFSET (DirectiveToken));

        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "#%s: Unknown directive\n",
            Gbl_CurrentLineNumber, DirectiveToken);
        return;
    }

    /* TBD: Need a faster way to do this: */

    if ((Directive == PR_DIRECTIVE_ELIF) ||
        (Directive == PR_DIRECTIVE_ELSE) ||
        (Directive == PR_DIRECTIVE_ENDIF))
    {
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID "Begin #%s\n",
            Gbl_CurrentLineNumber, Gbl_DirectiveInfo[Directive].Name);
    }

    /*
     * Need to always check for #else, #elif, #endif regardless of
     * whether we are ignoring the current code block, since these
     * are conditional code block terminators.
     */
    switch (Directive)
    {
    case PR_DIRECTIVE_ELIF:
        *IgnoringThisCodeBlock = !(*IgnoringThisCodeBlock);
        if (*IgnoringThisCodeBlock == TRUE)
        {
            /* Not executing the ELSE part -- all done here */
            return;
        }

        /* Will execute the ELSE..IF part */

        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "#elif - Executing else block\n",
            Gbl_CurrentLineNumber);
        Directive = PR_DIRECTIVE_IF;
        break;

    case PR_DIRECTIVE_ELSE:
        *IgnoringThisCodeBlock = !(*IgnoringThisCodeBlock);
        return;

    case PR_DIRECTIVE_ENDIF:
        *IgnoringThisCodeBlock = FALSE;
        Gbl_IfDepth--;
        if (Gbl_IfDepth < 0)
        {
            PrError (ASL_ERROR, ASL_MSG_ENDIF_MISMATCH,
                THIS_TOKEN_OFFSET (DirectiveToken));
            Gbl_IfDepth = 0;
        }
        return;

    default:
        break;
    }

    /*
     * At this point, if we are ignoring the current code block,
     * do not process any more directives (i.e., ignore them also.)
     */
    if (*IgnoringThisCodeBlock == TRUE)
    {
        return;
    }

    DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID "Begin #%s\n",
        Gbl_CurrentLineNumber, Gbl_DirectiveInfo[Directive].Name);

    /* Most directives have at least one argument */

    if (Gbl_DirectiveInfo[Directive].ArgCount == 1)
    {
        Token = PrGetNextToken (NULL, PR_TOKEN_SEPARATORS, Next);
        if (!Token)
        {
            goto SyntaxError;
        }
    }

    switch (Directive)
    {
    case PR_DIRECTIVE_DEFINE:
        /*
         * By definition, if first char after the name is a paren,
         * this is a function macro.
         */
        TokenOffset = Token - Gbl_MainTokenBuffer + strlen (Token);
        if (*(&Gbl_CurrentLineBuffer[TokenOffset]) == '(')
        {
#ifndef MACROS_SUPPORTED
            AcpiOsPrintf ("%s ERROR - line %u: #define macros are not supported yet\n",
                Gbl_CurrentLineBuffer, Gbl_CurrentLineNumber);
            exit(1);
#else
            PrAddMacro (Token, Next);
#endif
        }
        else
        {
            /* Use the remainder of the line for the #define */

            Token2 = *Next;
            if (Token2)
            {
                while ((*Token2 == ' ') || (*Token2 == '\t'))
                {
                    Token2++;
                }
                End = Token2;
                while (*End != '\n')
                {
                    End++;
                }
                *End = 0;
            }
            else
            {
                Token2 = "";
            }
#if 0
            Token2 = PrGetNextToken (NULL, "\n", /*PR_TOKEN_SEPARATORS,*/ Next);
            if (!Token2)
            {
                Token2 = "";
            }
#endif
            DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
                "New #define: %s->%s\n",
                Gbl_CurrentLineNumber, Token, Token2);

            PrAddDefine (Token, Token2, FALSE);
        }
        break;

    case PR_DIRECTIVE_ERROR:
        /* TBD compiler should abort */
        /* Note: No macro expansion */

        PrError (ASL_ERROR, ASL_MSG_ERROR_DIRECTIVE,
            THIS_TOKEN_OFFSET (Token));
        break;

    case PR_DIRECTIVE_IF:
        TokenOffset = Token - Gbl_MainTokenBuffer;

        /* Need to expand #define macros in the expression string first */

        Status = PrResolveIntegerExpression (
            &Gbl_CurrentLineBuffer[TokenOffset-1], &Value);
        if (ACPI_FAILURE (Status))
        {
            return;
        }

        if (!Value)
        {
            *IgnoringThisCodeBlock = TRUE;
        }

        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "Resolved #if: %8.8X%8.8X %s\n",
            Gbl_CurrentLineNumber, ACPI_FORMAT_UINT64 (Value),
            *IgnoringThisCodeBlock ? "<Skipping Block>" : "<Executing Block>");

        Gbl_IfDepth++;
        break;

    case PR_DIRECTIVE_IFDEF:
        if (!PrMatchDefine (Token))
        {
            *IgnoringThisCodeBlock = TRUE;
        }

        Gbl_IfDepth++;
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "Start #ifdef %s\n", Gbl_CurrentLineNumber,
            *IgnoringThisCodeBlock ? "<Skipping Block>" : "<Executing Block>");
        break;

    case PR_DIRECTIVE_IFNDEF:
        if (PrMatchDefine (Token))
        {
            *IgnoringThisCodeBlock = TRUE;
        }

        Gbl_IfDepth++;
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "Start #ifndef %2.2X\n", Gbl_CurrentLineNumber,
            *IgnoringThisCodeBlock, Gbl_CurrentLineNumber);
        break;

    case PR_DIRECTIVE_INCLUDE:
        Token = PrGetNextToken (NULL, " \"<>", Next);
        if (!Token)
        {
            goto SyntaxError;
        }

        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "Start #include file \"%s\"\n", Gbl_CurrentLineNumber,
            Token, Gbl_CurrentLineNumber);

        PrOpenIncludeFile (Token);
        break;

    case PR_DIRECTIVE_LINE:
        TokenOffset = Token - Gbl_MainTokenBuffer;

        Status = PrResolveIntegerExpression (
            &Gbl_CurrentLineBuffer[TokenOffset-1], &Value);
        if (ACPI_FAILURE (Status))
        {
            return;
        }

        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "User #line invocation %s\n", Gbl_CurrentLineNumber,
            Token);

        /* Update local line numbers */

        Gbl_CurrentLineNumber = (UINT32) Value;
        Gbl_PreviousLineNumber = 0;

        /* Emit #line into the preprocessor file */

        FlPrintFile (ASL_FILE_PREPROCESSOR, "#line %u \"%s\"\n",
            Gbl_CurrentLineNumber, Gbl_Files[ASL_FILE_INPUT].Filename);
        break;

    case PR_DIRECTIVE_PRAGMA:
        /* Only "#pragma message" supported at this time */

        if (strcmp (Token, "message"))
        {
            PrError (ASL_ERROR, ASL_MSG_UNKNOWN_PRAGMA,
                THIS_TOKEN_OFFSET (Token));
            return;
        }

        Token = PrGetNextToken (NULL, PR_TOKEN_SEPARATORS, Next);
        if (!Token)
        {
            goto SyntaxError;
        }

        TokenOffset = Token - Gbl_MainTokenBuffer;
        AcpiOsPrintf ("%s\n", &Gbl_CurrentLineBuffer[TokenOffset]);
        break;

    case PR_DIRECTIVE_UNDEF:
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "#undef: %s\n", Gbl_CurrentLineNumber, Token);

        PrRemoveDefine (Token);
        break;

    case PR_DIRECTIVE_WARNING:
        PrError (ASL_WARNING, ASL_MSG_ERROR_DIRECTIVE,
            THIS_TOKEN_OFFSET (Token));
        break;

    default:
        /* Should never get here */
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "Unrecognized directive: %u\n",
            Gbl_CurrentLineNumber, Directive);
        break;
    }

    return;


SyntaxError:

    PrError (ASL_ERROR, ASL_MSG_DIRECTIVE_SYNTAX,
        THIS_TOKEN_OFFSET (DirectiveToken));
    return;
}


/*******************************************************************************
 *
 * FUNCTION:    PrMatchDirective
 *
 * PARAMETERS:  Directive           - Pointer to directive name token
 *
 * RETURN:      Index into command array, -1 if not found
 *
 * DESCRIPTION: Lookup the incoming directive in the known directives table.
 *
 ******************************************************************************/

static int
PrMatchDirective (
    char                    *Directive)
{
    int                     i;


    if (!Directive || Directive[0] == 0)
    {
        return (ASL_DIRECTIVE_NOT_FOUND);
    }

    for (i = 0; Gbl_DirectiveInfo[i].Name; i++)
    {
        if (!strcmp (Gbl_DirectiveInfo[i].Name, Directive))
        {
            return (i);
        }
    }

    return (ASL_DIRECTIVE_NOT_FOUND);    /* Command not recognized */
}
