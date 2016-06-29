//////////////////////////////////////////////////////////////////////////
/// \defgroup McsfLogClient
///  Copyright, (c) Shanghai United Imaging healthcare Inc., 2011
///  All rights reserved.
///  \author        xiaotao.yang 
///  \email         mailto:xiaotao.yang@united-imaging.com
///  \file          mcsf_config_xml_parse.h
///  \brief         parse the log client xml configuration file
///  \date          September. 20, 2011
///
///   \revised by  xiaotao.yang  mailto:xiaotao.yang@united-imaging.com
///   \date   Nov.20, 2011
///  \{
//////////////////////////////////////////////////////////////////////////
#include "mcsf_config_xml_parse.h"
#include "study_series_command_request_args.h"
#include <string>
#include <sstream>
MCSF_DJ2DENGINE_BEGIN_NAMESPACE
	
/////////////////////////////////////////////////////////////////
///	\brief: constructor
/////////////////////////////////////////////////////////////////

CConfigXmlParse::CConfigXmlParse()
{
	try
	{
		m_pConfigXmlDocument = new TiXmlDocument();
		m_pXmlElement = NULL;
		m_pRootXmlElement = NULL;
	}
	catch (...)
	{
	}
}

/////////////////////////////////////////////////////////////////
///	\brief: destructor
/////////////////////////////////////////////////////////////////
CConfigXmlParse::~CConfigXmlParse()
{
	try
	{
		if (NULL != m_pConfigXmlDocument)
		{
			delete m_pConfigXmlDocument;
			m_pConfigXmlDocument = NULL;
		}
	}
	catch (...)
	{
	}
}

/////////////////////////////////////////////////////////////////
///	\brief: load the configuration file
///	\param:
///			[in] const std::string& strConfigFile: the configuration file
///	\return: true:  success to load configuration file,
///          false: fail to load configuration file.
/////////////////////////////////////////////////////////////////
bool CConfigXmlParse::LoadConfigXmlFile(const std::string& strConfigFile)
{
	try
	{
		if(strConfigFile.empty())
		{
			return false;
		}
		return m_pConfigXmlDocument->LoadFile(strConfigFile.c_str());
	}
	catch (std::exception &ex)
	{
		std::cout << "SW_LOGGER ERROR: " 
			<< "Failed CLogXmlParse::LoadConfigXmlFile !" << ex.what()
			<< std::endl;
		return false;
	}
	catch(...)
	{
		std::cout << "SW_LOGGER ERROR: " 
			<< "Failed CLogXmlParse::LoadConfigXmlFile!"
			<< std::endl;
		return false;
	}
}

/////////////////////////////////////////////////////////////////
///	\brief: load the configuration file
///	\param:
///			[in] const std::string& strXmlElementName: the xml tag
///			[out] const std::string& strConfigFile: the content of the tag
///	\return: true: success to get tag content , false:fail to get tag content
/////////////////////////////////////////////////////////////////
bool CConfigXmlParse::GetContentByXmlTag(const std::string& strXmlElementName, 
	std::string& strXmlElementContent)
{
	try
	{
		if(strXmlElementName.empty())
		{
			return false;
		}

		m_pRootXmlElement = m_pConfigXmlDocument->RootElement();
		if (NULL == m_pRootXmlElement)
		{
			return false;
		}
		m_pXmlElement = m_pRootXmlElement->FirstChildElement();
		while(m_pXmlElement)
		{
			std::string strElementName = m_pXmlElement->Value();
			if(0 == strXmlElementName.compare(strElementName))
			{           
				const char* strTempString = m_pXmlElement->GetText();
				if (NULL == strTempString)
				{
					return false;
				}

				strXmlElementContent = strTempString; 
				return true;
			}
			m_pXmlElement = m_pXmlElement->NextSiblingElement();
		}

		return false;
	}
	catch (...)
	{
		std::cout << "SW_LOGGER ERROR: "
			<< "Failed CLogXmlParse::GetContentByXmlTag"
			<< std::endl;
		return false;
	}
}

/////////////////////////////////////////////////////////////////
///	\brief: retrieve the relative value of a property
///          a given tag from the configuration file
///	\param:
///			[in] const std::string& strElemName: the xml tag
///         [in] const std::string &strProperty: the property name
///			[out] std::string& strPropertyValue: the value of the property
///	\return: true: retrieve the value of a given property successfully
///          false: fail to retrieve the value
/////////////////////////////////////////////////////////////////
bool CConfigXmlParse::GetPropertyValueByXmlTag(const std::string &strElemName
	, const std::string &strProperty, std::string &strPropertyValue)
{
	try
	{
		if (strElemName.empty() || strProperty.empty())
		{
			return false;
		}

		m_pRootXmlElement = m_pConfigXmlDocument->RootElement();
		if (!m_pRootXmlElement)
		{
			return false;
		}

		m_pXmlElement = m_pRootXmlElement->FirstChildElement();
		while (m_pXmlElement) 
		{
			std::string strTempElemName = m_pXmlElement->Value();
			if (0 == strElemName.compare(strTempElemName)) 
			{
				/// We find the element
				/// then we retrieve its strProperty value
				strPropertyValue = m_pXmlElement->Attribute(strProperty.c_str());
				if (0 == strPropertyValue.length())
				{
					return false;
				}

				return true;
			}

			/// strElemName is not the same as m_pElem->Value()
			/// try its sibling
			m_pXmlElement = m_pXmlElement->NextSiblingElement();
		}

		return false;
	}
	catch (...)
	{
		std::cerr << "SW_LOGGER ERROR" << "CLogXmlParse::GetPropertyValueByXmlTag Failed!"
				  << std::endl;

		return false;
	}
}

bool CConfigXmlParse::FindContentByXmlTag(const std::string& strXmlElementName, std::string& strXmlElementContent)
{
	try
	{
		if(strXmlElementName.empty())
		{
			return false;
		}

		m_pRootXmlElement = m_pConfigXmlDocument->RootElement();
		if (NULL == m_pRootXmlElement)
		{
			return false;
		}
		return SearchXmlTag(m_pRootXmlElement, strXmlElementName, strXmlElementContent);
	}
	catch (...)
	{
		std::cout << "SW_LOGGER ERROR: "
			<< "Failed CLogXmlParse::GetContentByXmlTag"
			<< std::endl;
		return false;
	}
}

bool CConfigXmlParse::SearchXmlTag(TiXmlElement *pElement, const std::string &strXmlElementName, std::string &strXmlElementContent)
{
	TiXmlElement *pChildElem = pElement->FirstChildElement();
	while(pChildElem)
	{
		std::string strElementName = pChildElem->Value();
		if(0 == strXmlElementName.compare(strElementName))
		{           
			const char* strTempString = pChildElem->GetText();
			if (NULL == strTempString)
			{
				return false;
			}

			strXmlElementContent = strTempString; 
			return true;
		}

		bool b = SearchXmlTag(pChildElem, strXmlElementName, strXmlElementContent);
		if (b)
			return true;

		pChildElem = pChildElem->NextSiblingElement();
	}
	
	return false;
}

bool CSiteCommentsConfig::BuildSiteCommentsRequestArgs(SiteCustComSettingsRequestArgs *pRequestArgs, const std::string &strContent)
{
	TiXmlDocument doc;
	const char *pRet = doc.Parse(strContent.c_str(), 0, TIXML_ENCODING_UTF8);
	if (pRet != NULL) return false;

	pRequestArgs->ClearComments();

	std::string strPosKey;
	COMMENT_WINDOW_POSITION pos;
	TiXmlElement *pRootElement = doc.RootElement();

	TiXmlElement *pPosElement = pRootElement->FirstChildElement();
	while (pPosElement)
	{
		strPosKey = pPosElement->Value();
		if (0 == strPosKey.compare("TopLeft"))
		{
			pos = VP_LEFT_TOP;
		}
		else if (0 == strPosKey.compare("TopRight"))
		{
			pos = VP_RIGHT_TOP;
		}
		else if (0 == strPosKey.compare("BottomLeft"))
		{
			pos = VP_LEFT_BOTTOM;
		}
		else if (0 == strPosKey.compare("BottomRight"))
		{
			pos = VP_RIGHT_BOTTOM;
		}
		else
		{
			pPosElement = pPosElement->NextSiblingElement();
			continue;
		}
		TiXmlElement *pItemsGroup = pPosElement->FirstChildElement();
		while (pItemsGroup)
		{
			CustomizedPosDcmTagCommentLine commLine;

			TiXmlElement *pItem = pItemsGroup->FirstChildElement();
			while (pItem)
			{
				CustomizedPosDcmTagComment commTag;

				// TO get item property
				std::string strTag = pItem->Attribute("Tag");
				if (strTag.length() != 8) continue;

				int group = 0, elemt = 0;

				std::istringstream hexstr(strTag.substr(0, 4));
				hexstr >> std::hex >> group;

				hexstr.clear();
				hexstr.str(strTag.substr(4));
				hexstr >> std::hex >> elemt;
				std::string strVal = pItem->Attribute("Name");

				std::string strFontName;
				int fontSize;
				Gdiplus::Color color;
				std::string strTxtFormat;
				StringFormatter valueFormatter;
				// to get it's child
				TiXmlElement *pItemChild = pItem->FirstChildElement();
				while(pItemChild)
				{
					std::string strKey = pItemChild->Value();
				
					std::string strText = pItemChild->GetText();
					if (0 == strKey.compare("FontFamily"))
					{
						strFontName = strText;
					}
					else if (0 == strKey.compare("FontSize"))
					{
						fontSize = atoi(strText.c_str());
					}
					else if (0 == strKey.compare("ForegroundColor"))
					{
						int a = -1, r = -1, g = -1, b = -1;

						char *sz = const_cast<char*>(strText.c_str());
						char *szToken = strtok(sz, ",");
						while (szToken)
						{
							if (a == -1)
							{
								a = atoi(szToken);
							}
							else if (r == -1)
							{
								r = atoi(szToken);
							}
							else if (g == -1)
							{
								g = atoi(szToken);
							}
							else if (b == -1)
							{
								b = atoi(szToken);
							}
							else break;
							szToken = strtok(NULL, ",");
						}

						color = Color(a, r, g, b);
					}
					else if (0 == strKey.compare("Format"))
					{
						std::string strAtt = pItemChild->Attribute("Type");
						std::string strFormat = pItemChild->GetText();
						if (0 == strAtt.compare("String"))
						{
							valueFormatter = StringFormatter(strFormat, FORMATTER_SRC_STRING);
						}
						else if (0 == strAtt.compare("Double"))
						{
							valueFormatter = StringFormatter(strFormat, FORMATTER_SRC_DOUBLE);
						}
						else if (0 == strAtt.compare("DateTime"))
						{
							valueFormatter = StringFormatter(strFormat, FORMATTER_SRC_DATETIME);
						}
						
					}
				
					pItemChild = pItemChild->NextSiblingElement();
				}

				commTag.SetDcmTagComment(pos, group, elemt, color, fontSize, strFontName, valueFormatter);
				commLine.AddDcmTagCommentItem(commTag);
				pItem = pItem->NextSiblingElement();
			}
			pItemsGroup = pItemsGroup->NextSiblingElement();

			pRequestArgs->AddCustComment(commLine);
		}
		
		pPosElement = pPosElement->NextSiblingElement();
	}

	
	return true;
}

MCSF_DJ2DENGINE_END_NAMESPACE
