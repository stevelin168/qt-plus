
// Qt
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QRegExp>
#include <QMutexLocker>
#include <QDebug>

// Application
#include "QMLAnalyzer.h"
#include "QMLItem.h"
#include "QMLFunction.h"

//-------------------------------------------------------------------------------------------------

/*!
    \class QMLAnalyzer
    \inmodule unis-lib
    \brief A QML analyzer (limited to the scope of a file). \br

    \section1 How it works
    This class runs checks over all contents of a QML file using a grammar file in XML format.

    \section1 Format of the XML grammar file.

    \section2 \c <Macro> tag
    It is possible to define a macro. A macro is a name and a value, and will be referenced in any text using a $ symbol before and after its name.
    Example of a macro declaration:
    \code
    <Macro Name="CamelCasingRegExp" Value="([a-z])([a-zA-Z0-9]*)"/>
    \endcode
    Here the macro is named CamelCasingRegExp and its value is "([a-z])([a-zA-Z0-9]*)". It can be used in any attribute like so:
    \code
    <Check Class="QMLVariableDeclaration">
        <Accept Member="name" RegExp="$CamelCasingRegExp$" Text="Only camel casing allowed in names"/>
    </Check>
    \endcode

    \section2 \c <Check> tag
    Defines a set of rules to apply to a particular QML Tree class. Please refer to the "List of classes and their members" section.

    \section3 \c <Accept> and \c <Reject> tags, inner tags of \c <Check>
    These tags define rules that should be checked. An <Accept> tag will yield failure if its contents evaluate to false. A <Reject> tag will yield failure if its contents evaluate to true.

    For instance, suppose we want to check (and reject) any hardcoded color strings. The rule will be defined as:
    \code
    <Check Class="QMLPropertyAssignment">
        <Reject Member="content" Class="QMLEntity" Text="No hardcoded colors allowed">
        </Reject>
    </Check>
    \endcode
    First, we begin a \c <Check> tag for the class \c QMLPropertyAssignment, because this is what is stored in the QML tree for an expression like \c color: "#FF0000".
    Second, we use a \c <Reject> tag because we want to yield a failure when the rule evaluates to \c true.
    In the \c <Reject> tag we use the following attributes:
    \list
        \li \c Member is set to \c content : we are interested by the contents of the assignment.
        \li \c Class is set to \c QMLEntity : we want something more complex than a simple value in the assignment, like a \c QMLIdentifier or a \c QMLBinaryOperation.
        \li \c Text is set to \c No \c hardcoded \c colors \c allowed : that's what we want to tell the user when the check yields a failure.
    \endlist
    Now this is not enough because this rule would run through ALL property assignments in the file, but we want only \c color properties to be processed.
    That's when the \c <Condition> tag comes in. It allows us to place conditions, thus refining the rule.
    \code
    <Check Class="QMLPropertyAssignment">
        <Reject Member="content" Class="QMLEntity" Text="No hardcoded colors allowed">
            <Condition Member="name" Value="color"/>
            <Condition Member="content" Value="transparent" Negate="true"/>
        </Reject>
    </Check>
    \endcode
    The conditions tell the analyzer that the rule should be run only when:
    \list
        \li The class member \c name is equal to \c color
        \li The class member \c content is not equal to \c "transparent" (the Negate attributes inverses the logic)
    \endlist

    \section4 Attributes of the \c <Accept> and \c <Reject> tags
    \list
    \li Member - Defines the member that we want to check. Please refer to the "List of classes and their members" section.
    \li Class - Defines the class name.
    \li Value - Makes the analyzer compare this value with the value of \c Member.
    \li RegExp - Makes the analyzer compare this regular expression with the value of \c Member.
    \li Count - Makes the analyzer count the contents of \c Member.
    \li NestedCount - Makes the analyzer recursively count the occurence of this class.
    \li Text - Defines the text to output when the rule yields failure.
    \endlist

    \section4 \c Attributes of the \c <Condition> tag, inner tag of \c <Accept> and \c <Reject>
    \list
    \li Member - Defines the member that we want to check. Please refer to the "List of classes and their members" section.
    \li Value - Makes the analyzer compare this value with the value of \c Member.
    \li Negate - When equal to \c true, reverses the logic of the condition test.
    \li Operation (Optional) - Specifies the type of operation to do on \c Member and \c Value.
        \list
            \li <empty> - Yields \c true if \c Member is equal to \c Value
            \li Contains - Yields \c true if \c Member contains \c Value
        \endlist
    \endlist

    \section1 List of classes and their members

    \list

    \li QMLEntity - The base of all QML Tree classes

    \li QMLComplexEntity - The base class of QML Tree classes that have more than one content
    \list
    \li name - The name of the entity
    \endlist

    \li QMLIdentifier (extends QMLEntity) - A string that is an identifier.

    \li QMLImport - An import statement like [ import QtQuick 2.5 ]
    \list
    \li name - The name following the "import" token, either an identifier or a path in a string
    \li version - The version following the name (if any)
    \li as - The name following the "as" statement if any
    \endlist

    \li QMLItem (extends QMLComplexEntity) - A QML item like [ Rectangle ]
    \list
    \li name - The name of the item
    \endlist

    \li QMLPropertyDeclaration - A property declaration like [ property color myColor ]
    \list
    \li type - The type of the property
    \li name - The name of the property
    \li content - The contents of the property (can be any class extending QMLEntity)
    \endlist

    \li QMLPropertyAssignment (extends QMLPropertyDeclaration) - A property assignement like [ id: root ]
    \list
    \li type - The type of the property
    \li name - The name of the property
    \li content - The contents of the property (can be any class extending QMLEntity)
    \endlist

    \li QMLFunction - A function declaration
    \list
    \li name - The name of the function
    \li parameters - The parameters of the function
    \li content - The contents of the function
    \endlist

    \li QMLIf - An if statement
    \list
    \li condition - The expression specified between the '(' and ')' tokens
    \li then - The statement executed when the condition evaluates to \c TRUE
    \li else - The statement executed when the condition evaluates to \c FALSE
    \endlist

    \li QMLConditional (extends QMLIf) - A conditional expression (a ? 0 : 1)
    \list
    \li condition - The expression specified before the '?' token
    \li then - The statement executed when the condition evaluates to \c TRUE
    \li else - The statement executed when the condition evaluates to \c FALSE
    \endlist

    \li QMLFor - A for loop
    \list
    \li initialization - The statement before the first ';' token
    \li condition - The expression before the second ';' token
    \li incrementation - The statement after the second ';' token
    \li content - The contents of the loop
    \endlist

    \li QMLForIn - A for-in loop
    \list
    \li variable - The variable
    \li expression - The expression filling the variable
    \li content - The contents of the loop
    \endlist

    \li QMLSwitch - A switch/case statement
    \list
    \li expression - The expression
    \li cases - A QMLComplexEntity containing the cases
    \endlist

    \li QMLBinaryOperation
    \list
    \li left - The expression that is left of the operator
    \li right - The expression that is right of the operator
    \endlist

    \endlist

    \section1 The grammar file XSD
    \code
    <?xml version="1.0" encoding="UTF-8"?>
    <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
      <xs:element name="Condition">
        <xs:complexType>
          <xs:attribute name="Member" type="xs:string">
            <xs:annotation>
              <xs:documentation>
                Defines the name of the member of the current class we want to work with.
              </xs:documentation>
            </xs:annotation>
          </xs:attribute>
          <xs:attribute name="Empty" type="xs:string">
            <xs:annotation>
              <xs:documentation>
                This can contain 'true' or 'false'.
                Evaluates to true (or false) if the 'Member' of the class is an empty string.
              </xs:documentation>
            </xs:annotation>
          </xs:attribute>
          <xs:attribute name="Value" type="xs:string">
            <xs:annotation>
              <xs:documentation>
                If this is defined, it will be compared to the string value of 'Member'.
              </xs:documentation>
            </xs:annotation>
          </xs:attribute>
          <xs:attribute name="Negate" type="xs:string">
            <xs:annotation>
              <xs:documentation>
                If this is defined and equal to 'true', it will reverse the effect of the condition.
              </xs:documentation>
            </xs:annotation>
          </xs:attribute>
        </xs:complexType>
      </xs:element>
      <xs:element name="Root">
        <xs:annotation>
          <xs:documentation>
            The root of the document.
          </xs:documentation>
        </xs:annotation>
        <xs:complexType>
          <xs:choice minOccurs="0" maxOccurs="unbounded">
            <xs:element name="Macro">
              <xs:complexType>
                <xs:attribute name="Name" type="xs:string" />
                <xs:attribute name="Value" type="xs:string" />
              </xs:complexType>
            </xs:element>
            <xs:element name="Check">
              <xs:complexType>
                <xs:sequence>
                  <xs:element name="Reject" minOccurs="0" maxOccurs="unbounded">
                    <xs:complexType>
                      <xs:sequence>
                        <xs:element ref="Condition" minOccurs="0" maxOccurs="unbounded" />
                      </xs:sequence>
                      <xs:attribute name="Member" type="xs:string">
                        <xs:annotation>
                          <xs:documentation>
                            Defines the name of the member of the class we want to work with.
                          </xs:documentation>
                        </xs:annotation>
                      </xs:attribute>
                      <xs:attribute name="RegExp" type="xs:string">
                        <xs:annotation>
                          <xs:documentation>
                            If defined, must contain a regular expression.
                            The analyzer will match it against the string value of 'Member'.
                          </xs:documentation>
                        </xs:annotation>
                      </xs:attribute>
                      <xs:attribute name="Count" type="xs:string">
                        <xs:annotation>
                          <xs:documentation>
                            If defined, must contain an integer.
                            The analyzer will count the contents of 'Member' (if any).
                          </xs:documentation>
                        </xs:annotation>
                      </xs:attribute>
                      <xs:attribute name="NestedCount" type="xs:string">
                        <xs:annotation>
                          <xs:documentation>
                            If defined, must contain an integer.
                            The analyzer will count the occurence of the current class in a recursive manner.
                          </xs:documentation>
                        </xs:annotation>
                      </xs:attribute>
                      <xs:attribute name="Path" type="xs:string">
                        <xs:annotation>
                          <xs:documentation>
                            If defined, must contain 'Exists'.
                            The analyzer will check that the path contained in 'Member' exists.
                          </xs:documentation>
                        </xs:annotation>
                      </xs:attribute>
                      <xs:attribute name="List" type="xs:string">
                        <xs:annotation>
                          <xs:documentation>
                            If defined, must contain a list of comma-separated strings.
                            The analyzer will check that the string value of 'Member' is contained in this list.
                          </xs:documentation>
                        </xs:annotation>
                      </xs:attribute>
                      <xs:attribute name="Text" type="xs:string">
                        <xs:annotation>
                          <xs:documentation>
                            The text to show to the user when this reject is yielded.
                          </xs:documentation>
                        </xs:annotation>
                      </xs:attribute>
                    </xs:complexType>
                  </xs:element>
                  <xs:element name="Accept" minOccurs="0" maxOccurs="unbounded">
                    <xs:complexType>
                      <xs:sequence>
                        <xs:element ref="Condition" minOccurs="0" maxOccurs="unbounded" />
                      </xs:sequence>
                      <xs:attribute name="Member" type="xs:string" />
                      <xs:attribute name="RegExp" type="xs:string" />
                      <xs:attribute name="Count" type="xs:string" />
                      <xs:attribute name="NestedCount" type="xs:string" />
                      <xs:attribute name="Text" type="xs:string" />
                      <xs:attribute name="Path" type="xs:string" />
                      <xs:attribute name="List" type="xs:string" />
                    </xs:complexType>
                  </xs:element>
                </xs:sequence>
                <xs:attribute name="Class" type="xs:string">
                  <xs:annotation>
                    <xs:documentation>
                      The name of the class we want to run checks on.
                    </xs:documentation>
                  </xs:annotation>
                </xs:attribute>
              </xs:complexType>
            </xs:element>
          </xs:choice>
        </xs:complexType>
      </xs:element>
    </xs:schema>
    \endcode
*/

//-------------------------------------------------------------------------------------------------
// XML Grammar File Tokens

#define ANALYZER_TOKEN_MACRO        "Macro"
#define ANALYZER_TOKEN_NAME         "Name"
#define ANALYZER_TOKEN_CHECK        "Check"
#define ANALYZER_TOKEN_CLASS        "Class"
#define ANALYZER_TOKEN_LIST         "List"
#define ANALYZER_TOKEN_COUNT        "Count"
#define ANALYZER_TOKEN_MEMBER       "Member"
#define ANALYZER_TOKEN_NESTED_COUNT "NestedCount"
#define ANALYZER_TOKEN_ACCEPT       "Accept"
#define ANALYZER_TOKEN_REJECT       "Reject"
#define ANALYZER_TOKEN_TEXT         "Text"
#define ANALYZER_TOKEN_TYPE         "Type"
#define ANALYZER_TOKEN_VALUE        "Value"
#define ANALYZER_TOKEN_REGEXP       "RegExp"
#define ANALYZER_TOKEN_PATH         "Path"
#define ANALYZER_TOKEN_OPERATION    "Operation"
#define ANALYZER_TOKEN_EXISTS       "Exists"
#define ANALYZER_TOKEN_CONTAINS     "Contains"
#define ANALYZER_TOKEN_CONDITION    "Condition"
#define ANALYZER_TOKEN_NEGATE       "Negate"
#define ANALYZER_TOKEN_EMPTY        "Empty"
#define ANALYZER_TOKEN_FILE_NAME    "filename"
#define ANALYZER_TOKEN_TRUE         "true"
#define ANALYZER_TOKEN_FALSE        "false"

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CCodeAnalyzer.
*/
QMLAnalyzer::QMLAnalyzer()
    : QThread(NULL)
    , m_mContextMutex(QMutex::Recursive)
    , m_pContext(nullptr)
    , m_bIncludeImports(false)
    , m_bIncludeSubFolders(false)
    , m_bRewriteFiles(false)
    , m_bRemoveUnreferencedSymbols(false)
    , m_bStopAnalyzeRequested(false)
{
    m_eEngine.globalObject().setProperty("analyzer", m_eEngine.newQObject(new QMLAnalyzerWrapper(this)));

    QFile fScript(":/beautify.js");
    if (fScript.open(QFile::ReadOnly))
    {
        m_sBeautifyScript = fScript.readAll();
        fScript.close();
    }
}

/*!
    Destroys a CCodeAnalyzer.
*/
QMLAnalyzer::~QMLAnalyzer()
{
    m_eEngine.globalObject().setProperty("analyzer", m_eEngine.newQObject(nullptr));

    if (m_pContext != nullptr)
    {
        delete m_pContext;
    }
}

/*!
    Sets the base folder member to \a sFolder.
*/
void QMLAnalyzer::setFolder(const QString& sFolder)
{
    m_sFolder = sFolder;
}

/*!
    Sets the base file member to \a sFileName.
*/
void QMLAnalyzer::setFile(const QString& sFileName)
{
    m_sFile = sFileName;
}

/*!
    Sets the include imports flag to \a bValue. If the include imports flag is \c true, the analyzer will parse all imported files in the parsed files.
*/
void QMLAnalyzer::setIncludeImports(bool bValue)
{
    m_bIncludeImports = bValue;
}

/*!
    Sets the include subfolders flag to \a bValue. If the include subfolders flag is \c true, the analyzer will process all subfolders of the given folder.
*/
void QMLAnalyzer::setIncludeSubFolders(bool bValue)
{
    m_bIncludeSubFolders = bValue;
}

/*!
    Sets the include rewrite files flag to \a bValue. If the rewrite files flag is \c true, the analyzer will overwrite the contents of the input file using auto-formatting.
*/
void QMLAnalyzer::setRewriteFiles(bool bValue)
{
    m_bRewriteFiles = bValue;
}

/*!
    Sets the remove unreferenced symbols flag to \a bValue. If the rewrite files flag is \c true, the analyzer will remove any unreferenced symbol.
*/
void QMLAnalyzer::setRemoveUnreferencedSymbols(bool bValue)
{
    m_bRemoveUnreferencedSymbols = bValue;
}

/*!
    Returns the name of the base folder.
*/
QString QMLAnalyzer::folder() const
{
    return m_sFolder;
}

/*!
    Returns the list of errors.
*/
const QVector<QMLAnalyzerError>& QMLAnalyzer::errors() const
{
    return m_vErrors;
}

/*!
    Returns the parsing context.
*/
QMLTreeContext* QMLAnalyzer::context()
{
    QMutexLocker locker(&m_mContextMutex);

    return m_pContext;
}

/*!
    Deletes the parsing context and allocates a new one.
*/
void QMLAnalyzer::clear()
{
    if (m_pContext != nullptr)
    {
        delete m_pContext;
    }

    m_pContext = new QMLTreeContext();
}

/*!
    Runs an analysis on the specified folder or file using \a xGrammar. Returns \c true on success.
*/
bool QMLAnalyzer::analyze(CXMLNode xGrammar)
{
    m_xGrammar = xGrammar;

    parseMacros();

    {
        QMutexLocker locker(&m_mContextMutex);

        if (m_pContext != nullptr)
        {
            delete m_pContext;
        }

        m_pContext = new QMLTreeContext();

        m_vErrors.clear();
    }

    connect(m_pContext, SIGNAL(parsingStarted(QString)), this, SIGNAL(parsingStarted(QString)), Qt::DirectConnection);
    connect(m_pContext, SIGNAL(parsingFinished(QString)), this, SIGNAL(parsingFinished(QString)), Qt::DirectConnection);
    connect(m_pContext, SIGNAL(importParsingStarted(QString)), this, SIGNAL(importParsingStarted(QString)), Qt::DirectConnection);

    if (m_sFolder.isEmpty() == false)
    {
        analyze_Recurse(m_sFolder);
    }
    else if (m_sFile.isEmpty() == false)
    {
        analyzeFile(m_sFile);
    }

    return true;
}

/*!
    Runs a threaded analyze on the specified folder or file using \a xGrammar. Returns \c true on success.
*/
void QMLAnalyzer::threadedAnalyze(CXMLNode xGrammar)
{
    if (isRunning() == false)
    {
        m_xGrammar = xGrammar;

        start();
    }
}

/*!
    Stops a threaded analyze if it is running.
*/
void QMLAnalyzer::stopThreadedAnalyze()
{
    if (isRunning())
    {
        m_bStopAnalyzeRequested = true;
        wait();
        m_bStopAnalyzeRequested = false;
    }
}

/*!
    Override the thread run method.
*/
void QMLAnalyzer::run()
{
    analyze(m_xGrammar);
}

void QMLAnalyzer::parseMacros()
{
    QVector<CXMLNode> vMacros = m_xGrammar.getNodesByTagName(ANALYZER_TOKEN_MACRO);

    m_mMacros.clear();

    foreach (CXMLNode xMacro, vMacros)
    {
        QString sName = xMacro.attributes()[ANALYZER_TOKEN_NAME];
        QString sValue = xMacro.attributes()[ANALYZER_TOKEN_VALUE];

        m_mMacros[sName] = sValue;
    }
}

/*!
    Returns \a sText with macro names replaced .with their respective value.
*/
QString QMLAnalyzer::processMacros(const QString& sText)
{
    QString sResult = sText;

    foreach (QString sMacro, m_mMacros.keys())
    {
        QString sFullMacroName = QString("$%1$").arg(sMacro);

        if (sResult.contains(sFullMacroName))
        {
            if (m_mMacros[sMacro].count() > 0)
            {
                QString sMacroValue = m_mMacros[sMacro];
                sResult.replace(sFullMacroName, sMacroValue);
            }
        }
    }

    return sResult;
}

/*!
    Runs an analysis on \a sFileName. Returns \c true on success.
*/
bool QMLAnalyzer::analyzeFile(const QString& sFileName)
{
    m_pContext->addFile(sFileName);

    m_pContext->setIncludeImports(m_bIncludeImports);

    if (m_pContext->parse() == QMLTreeContext::peSuccess)
    {
        QMLFile* pFile = m_pContext->fileByFileName(sFileName);

        runGrammar(sFileName, pFile);

        if (m_bRewriteFiles)
        {
            if (m_bRemoveUnreferencedSymbols)
            {
                pFile->removeUnreferencedSymbols(m_pContext);
            }

            QFile file(sFileName);

            if (file.open(QFile::WriteOnly))
            {
                m_sText.clear();
                QTextStream stream(&m_sText);

                pFile->toQML(stream);

                QJSValue output = m_eEngine.evaluate(m_sBeautifyScript);
                m_sText = output.toString();

                file.write(m_sText.toLatin1());
                file.close();
            }
        }
    }
    else
    {
        m_vErrors << m_pContext->error();

        emit analyzeError(m_vErrors.last());
    }

    return true;
}

/*!
    Runs an analysis on the files in \a sDirectory. Returns \c true on success.
*/
bool QMLAnalyzer::analyze_Recurse(QString sDirectory)
{
    if (m_bStopAnalyzeRequested)
        return false;

    QStringList slNameFilter;
    slNameFilter << "*.qml" << "*.js";

    QDir dDirectory(sDirectory);

    dDirectory.setFilter(QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Files);
    QStringList lFiles = dDirectory.entryList(slNameFilter);

    foreach (QString sFile, lFiles)
    {
        QString sFullName = QString("%1/%2").arg(sDirectory).arg(sFile);

        analyzeFile(sFullName);

        if (m_bStopAnalyzeRequested)
            return false;
    }

    if (m_bIncludeSubFolders)
    {
        dDirectory.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        QStringList lDirectories = dDirectory.entryList();

        foreach (QString sNewDirectory, lDirectories)
        {
            QString sFullName = QString("%1/%2").arg(sDirectory).arg(sNewDirectory);

            if (analyze_Recurse(sFullName) == false)
            {
                return false;
            }
        }
    }

    return true;
}

/*!
    Runs a check on the contents of \a pFile. \a sFileName is the full file name.
*/
void QMLAnalyzer::runGrammar(const QString& sFileName, QMLFile* pFile)
{
    foreach (QMLEntity* pEntity, pFile->contents())
    {
        runGrammar_Recurse(sFileName, pEntity);
    }
}

/*!
    Runs a check on the contents of \a pEntity. \a sFileName is the full file name.
*/
void QMLAnalyzer::runGrammar_Recurse(const QString& sFileName, QMLEntity* pEntity)
{
    if (pEntity == nullptr)
    {
        return;
    }

    // Check symbol usage if it is an item
    QMLItem* pItem = dynamic_cast<QMLItem*>(pEntity);

    if (pItem != nullptr)
    {
        QMap<QString, QMLEntity*> unusedProperties = pItem->unusedProperties();

        foreach (QString sKey, unusedProperties.keys())
        {
            outputError(sFileName, unusedProperties[sKey]->position(), "Unreferenced property");
        }
    }

    // Check symbol usage if it is a function
    QMLFunction* pFunction = dynamic_cast<QMLFunction*>(pEntity);

    if (pFunction != nullptr)
    {
        QMap<QString, QMLEntity*> unusedVariables = pFunction->unusedVariables();

        foreach (QString sKey, unusedVariables.keys())
        {
            outputError(sFileName, unusedVariables[sKey]->position(), "Unreferenced variable");
        }

        QMap<QString, QMLEntity*> unusedParameters = pFunction->unusedParameters();

        foreach (QString sKey, unusedParameters.keys())
        {
            outputError(sFileName, unusedParameters[sKey]->position(), "Unreferenced parameter");
        }
    }

    bool bHasRejects = false;

    QMap<QString, QMLEntity*> mMembers = pEntity->members();

    QVector<CXMLNode> vChecks = m_xGrammar.getNodesByTagName(ANALYZER_TOKEN_CHECK);

    foreach (CXMLNode xCheck, vChecks)
    {
        QString sClassName = xCheck.attributes()[ANALYZER_TOKEN_CLASS];

        if (pEntity->metaObject()->className() == sClassName)
        {
            QVector<CXMLNode> vAccepts = xCheck.getNodesByTagName(ANALYZER_TOKEN_ACCEPT);
            QVector<CXMLNode> vRejects = xCheck.getNodesByTagName(ANALYZER_TOKEN_REJECT);

            foreach (CXMLNode xReject, vRejects)
            {
                if (runGrammar_Reject(sFileName, sClassName, pEntity, xReject, false))
                    bHasRejects = true;
            }

            foreach (CXMLNode xAccept, vAccepts)
            {
                if (runGrammar_Reject(sFileName, sClassName, pEntity, xAccept, true))
                    bHasRejects = true;
            }
        }
    }

    if (bHasRejects == false)
    {
        foreach (QString sKey, mMembers.keys())
        {
            runGrammar_Recurse(sFileName, mMembers[sKey]);
        }

        QMLComplexEntity* pComplex = dynamic_cast<QMLComplexEntity*>(pEntity);

        if (pComplex != nullptr)
        {
            foreach (QMLEntity* pChildItem, pComplex->contents())
            {
                runGrammar_Recurse(sFileName, pChildItem);
            }
        }
    }
}

bool QMLAnalyzer::runGrammar_Reject(const QString& sFileName, const QString& sClassName, QMLEntity* pEntity, CXMLNode xRule, bool bInverseLogic)
{
    QString sMember = processMacros(xRule.attributes()[ANALYZER_TOKEN_MEMBER].toLower());
    QString sValue = processMacros(xRule.attributes()[ANALYZER_TOKEN_VALUE]);
    QString sType = processMacros(xRule.attributes()[ANALYZER_TOKEN_TYPE]);
    QString sText = processMacros(xRule.attributes()[ANALYZER_TOKEN_TEXT]);
    QString sNestedCount = processMacros(xRule.attributes()[ANALYZER_TOKEN_NESTED_COUNT]);
    QString sCount = processMacros(xRule.attributes()[ANALYZER_TOKEN_COUNT]);
    QString sRegExp = processMacros(xRule.attributes()[ANALYZER_TOKEN_REGEXP]);
    QString sPath = processMacros(xRule.attributes()[ANALYZER_TOKEN_PATH]);
    QString sList = processMacros(xRule.attributes()[ANALYZER_TOKEN_LIST]);
    QString sClass = processMacros(xRule.attributes()[ANALYZER_TOKEN_CLASS]);

    if (runGrammar_SatisfiesConditions(sFileName, sClassName, pEntity, xRule))
    {
        QMap<QString, QMLEntity*> mMembers = pEntity->members();

        // Check nested count
        if (sNestedCount.isEmpty() == false)
        {
            int iNestedCountAllowed = sNestedCount.toInt();

            if ((iNestedCountAllowed > 0) ^ bInverseLogic)
            {
                int iNestedCount = runGrammar_CountNested(sClassName, pEntity);

                if (iNestedCount > iNestedCountAllowed)
                {
                    outputError(sFileName, pEntity->position(), sText);
                    return true;
                }
            }
        }
        else if (mMembers.contains(sMember) && mMembers[sMember] != nullptr)
        {
            QString sMemberToString = mMembers[sMember]->toString();
            QString sMemberClass = mMembers[sMember]->metaObject()->className();

            sMemberToString = sMemberToString.replace("\"", "");

            // Check inclusion (or exclusion) in a list
            if (sList.isEmpty() == false)
            {
                QStringList lNames = sList.split(",");

                if (lNames.contains(sMemberToString) ^ bInverseLogic)
                {
                    outputError(sFileName, pEntity->position(), sText);
                    return true;
                }
            }
            // Check the class of the member
            else if (sClass.isEmpty() == false)
            {
                if ((sMemberClass == sClass) ^ bInverseLogic)
                {
                    outputError(sFileName, pEntity->position(), sText);
                    return true;
                }
            }
            // Check the path if requested
            else if (sPath.isEmpty() == false)
            {
                if (sPath == ANALYZER_TOKEN_EXISTS)
                {
                    QFileInfo tFileInfo(sFileName);
                    QString sDirectory = tFileInfo.absolutePath();
                    QDir tDirectory(sDirectory);
                    QString sFullImportPath = tDirectory.absoluteFilePath(sMemberToString);
                    QFileInfo tFullFileInfo(sFullImportPath);
                    QDir tFullImportDirectory(sFullImportPath);
                    bool bExists = false;

                    if (tFullFileInfo.exists())
                    {
                        bExists = true;
                    }

                    if (bExists == false)
                    {
                        if (tFullImportDirectory.exists())
                        {
                            bExists = true;
                        }
                    }

                    if ((bExists == true) ^ bInverseLogic)
                    {
                        outputError(sFileName, pEntity->position(), sText);
                        return true;
                    }
                }
            }
            // Match a regular expression if requested
            else if (sRegExp.isEmpty() == false && sMemberToString.isEmpty() == false)
            {
                QRegExp tRegExp(sRegExp);

                if ((tRegExp.exactMatch(sMemberToString)) ^ bInverseLogic)
                {
                    outputError(sFileName, pEntity->position(), sText);
                    return true;
                }
            }
            // Check the count if requested
            else if (sCount.isEmpty() == false)
            {
                int iCountToCheck = sCount.toInt();
                QMLComplexEntity* pComplex = dynamic_cast<QMLComplexEntity*>(mMembers[sMember]);

                if (pComplex != nullptr)
                {
                    if ((pComplex->contents().count() > iCountToCheck) ^ bInverseLogic)
                    {
                        outputError(sFileName, pEntity->position(), sText);
                        return true;
                    }
                }
            }
            // Check the type if requested
            else if (sType.isEmpty() == false)
            {
                QString sTypeToString = QMLType::typeToString(mMembers[sMember]->value().type());

                if ((sTypeToString == sType) ^ bInverseLogic)
                {
                    outputError(sFileName, pEntity->position(), sText);
                    return true;
                }
            }
            else
            {
                if ((sMemberToString == sValue) ^ bInverseLogic)
                {
                    outputError(sFileName, pEntity->position(), sText);
                    return true;
                }
            }
        }
    }

    return false;
}

bool QMLAnalyzer::runGrammar_SatisfiesConditions(const QString& sFileName, const QString& sClassName, QMLEntity* pEntity, CXMLNode xRule)
{
    QVector<CXMLNode> vConditions = xRule.getNodesByTagName(ANALYZER_TOKEN_CONDITION);

    QMap<QString, QMLEntity*> mMembers = pEntity->members();

    foreach (CXMLNode xCondition, vConditions)
    {
        QString sOperation = xCondition.attributes()[ANALYZER_TOKEN_OPERATION];
        QString sValue = xCondition.attributes()[ANALYZER_TOKEN_VALUE];
        QString sMember = xCondition.attributes()[ANALYZER_TOKEN_MEMBER].toLower();
        QString sEmpty = xCondition.attributes()[ANALYZER_TOKEN_EMPTY].toLower();
        QString sNegate = xCondition.attributes()[ANALYZER_TOKEN_NEGATE].toLower();

        if (mMembers.contains(sMember) && mMembers[sMember] != nullptr)
        {
            QString sMemberToString = mMembers[sMember]->toString();

            sMemberToString = sMemberToString.replace("\"", "");

            // Check the empty condition
            if (sValue.isEmpty() == false)
            {
                if (sMemberToString == sValue)
                {
                    if (sNegate == ANALYZER_TOKEN_TRUE)
                    {
                        return false;
                    }
                }
                else
                {
                    if (sNegate != ANALYZER_TOKEN_TRUE)
                    {
                        return false;
                    }
                }
            }
            else if (sEmpty.isEmpty() == false)
            {
                if ((sMemberToString.isEmpty() && (sEmpty == ANALYZER_TOKEN_TRUE)) == false)
                {
                    return false;
                }
            }
        }
        else
        {
            // Check the file name condition
            if (sMember == ANALYZER_TOKEN_FILE_NAME)
            {
                if (sOperation == ANALYZER_TOKEN_CONTAINS)
                {
                    if (sFileName.contains(sValue))
                    {
                        if (sNegate == ANALYZER_TOKEN_TRUE)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (sNegate != ANALYZER_TOKEN_TRUE)
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    if (sFileName == sValue)
                    {
                        if (sNegate == ANALYZER_TOKEN_TRUE)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (sNegate != ANALYZER_TOKEN_TRUE)
                        {
                            return false;
                        }
                    }
                }
            }
            else if (sEmpty.isEmpty() == false)
            {
                if (sEmpty != ANALYZER_TOKEN_TRUE)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

int QMLAnalyzer::runGrammar_CountNested(const QString& sClassName, QMLEntity* pEntity)
{
    int iCount = 0;

    if (pEntity != nullptr)
    {
        QMap<QString, QMLEntity*> mMembers = pEntity->members();

        foreach (QString sKey, mMembers.keys())
        {
            int iNewCount = runGrammar_CountNested(sClassName, mMembers[sKey]);

            if (iNewCount > iCount)
                iCount = iNewCount;
        }

        QMLComplexEntity* pComplex = dynamic_cast<QMLComplexEntity*>(pEntity);

        if (pComplex != nullptr)
        {
            foreach (QMLEntity* pChildItem, pComplex->contents())
            {
                int iNewCount = runGrammar_CountNested(sClassName, pChildItem);

                if (iNewCount > iCount)
                    iCount = iNewCount;
            }
        }

        if (pEntity->metaObject()->className() == sClassName)
        {
            iCount++;
        }
    }

    return iCount;
}

void QMLAnalyzer::outputError(const QString& sFileName, const QPoint& pPosition, const QString& sText)
{
    m_vErrors << QMLAnalyzerError(sFileName, pPosition, sText);

    emit analyzeError(m_vErrors.last());
}
