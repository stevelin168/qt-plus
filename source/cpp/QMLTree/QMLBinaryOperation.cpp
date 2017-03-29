
// Application
#include "QMLBinaryOperation.h"

//-------------------------------------------------------------------------------------------------

QMLBinaryOperation::QMLBinaryOperation(const QPoint& pPosition, QMLEntity* pLeft, QMLEntity* pRight, EOperator eOperator)
    : QMLEntity(pPosition)
    , m_pLeft(pLeft)
    , m_pRight(pRight)
    , m_eOperator(eOperator)
{
    if (m_pLeft != nullptr) m_pLeft->setParent(this);
    if (m_pRight != nullptr) m_pRight->setParent(this);
}

//-------------------------------------------------------------------------------------------------

QMLBinaryOperation::~QMLBinaryOperation()
{
    if (m_pLeft != NULL)
        delete m_pLeft;
    if (m_pRight != NULL)
        delete m_pRight;
}

//-------------------------------------------------------------------------------------------------

QMLEntity* QMLBinaryOperation::left() const
{
    return m_pLeft;
}

//-------------------------------------------------------------------------------------------------

QMLEntity* QMLBinaryOperation::right() const
{
    return m_pRight;
}

//-------------------------------------------------------------------------------------------------

QMLBinaryOperation::EOperator QMLBinaryOperation::oper() const
{
    return m_eOperator;
}

//-------------------------------------------------------------------------------------------------

QString QMLBinaryOperation::operatorToString(EOperator eOperator) const
{
    switch (eOperator)
    {
        case boNone:
            return "NONE";
        case boAssign:
            return "=";
        case boAdd:
            return "+";
        case boSub:
            return "-";
        case boMul:
            return "*";
        case boDiv:
            return "/";
        case boMod:
            return "%";
        case boAnd:
            return "&";
        case boOr:
            return "|";
        case boXor:
            return "^";
        case boLogicAnd:
            return "&&";
        case boLogicOr:
            return "||";
        case boEquals:
            return "==";
        case boEqualsCheck:
            return "===";
        case boNotEquals:
            return "!=";
        case boNotEqualsCheck:
            return "!==";
        case boLower:
            return "<";
        case boLowerOrEquals:
            return "<=";
        case boGreater:
            return ">";
        case boGreaterOrEquals:
            return ">=";
    }

    return "??";
}

//-------------------------------------------------------------------------------------------------

QMap<QString, QMLEntity*> QMLBinaryOperation::members()
{
    QMap<QString, QMLEntity*> vReturnValue;

    vReturnValue["Left"] = m_pLeft;
    vReturnValue["Right"] = m_pRight;

    return vReturnValue;
}

//-------------------------------------------------------------------------------------------------

void QMLBinaryOperation::toQML(QTextStream& stream, QMLTreeContext* pContext, QMLEntity* pParent, int iIdent)
{
    Q_UNUSED(pContext);
    Q_UNUSED(pParent);

    if (m_bIsParenthesized)
    {
        stream << " ( ";
    }

    if (m_pLeft != nullptr)
    {
        m_pLeft->toQML(stream, pContext, this, iIdent);
    }

    stream <<  QString(" %1 ").arg(operatorToString(m_eOperator));

    if (m_pRight != nullptr)
    {
        m_pRight->toQML(stream, pContext, this, iIdent);
    }

    if (m_bIsParenthesized)
    {
        stream << " ) ";
    }
}

//-------------------------------------------------------------------------------------------------

CXMLNode QMLBinaryOperation::toXMLNode(CXMLNodableContext* pContext, CXMLNodable* pParent)
{
    CXMLNode xNode = QMLEntity::toXMLNode(pContext, pParent);
    CXMLNode xLeft("Left");
    CXMLNode xRight("Right");

    xNode.attributes()["Operator"] = operatorToString(m_eOperator);

    if (m_pLeft != NULL)
        xLeft.nodes() << m_pLeft->toXMLNode(pContext, this);

    if (m_pRight != NULL)
        xRight.nodes() << m_pRight->toXMLNode(pContext, this);

    xNode.nodes() << xLeft;
    xNode.nodes() << xRight;

    return xNode;
}
