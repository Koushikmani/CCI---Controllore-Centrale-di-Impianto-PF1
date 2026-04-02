#include "GetCidKeysProgram.hpp"
#include "Arp/System/Commons/Logging.h"
#if ARP_ABI_VERSION_MAJOR < 2
#include "Arp/System/Core/ByteConverter.hpp"
#else
#include "Arp/Base/Core/ByteConverter.hpp"
#endif

#include "../lib/pugixml-1.15/src/pugixml.hpp"
#include "../lib/pugixml-1.15/src/pugixml.cpp"
#include "../lib/pugixml-1.15/src/pugiconfig.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace GetCidKeys
{

    // Struttura per estrazione dati dal file CID
    //
    struct udtCidData
    {
        std::string sName;
        std::string sFC;
        bool xTimestamp;
        bool xAlwaysUpdate;
        std::string rctlValue;
        bool xNDR;
        std::string rValue;
        bool IsString;

        // Aggiungiamo il confronto per sName (necessario per molti template STL)
        bool operator<(const udtCidData& altro) const {
            return sName < altro.sName;
        }
    };


    // Struttura per estrazione inizializzazione dati del file CID
    //
    struct udtCidInit
    {
        std::string sName;
        std::string Val;

        // Aggiungiamo il confronto per sName (necessario per molti template STL)
        bool operator<(const udtCidData& altro) const {
            return sName < altro.sName;
        }
    };






    // Procedura per estrazione delle inizializzazioni presenti nel file CID
    //
    void AppendInitValue(std::string LNKey, pugi::xml_node  xNodeType, std::vector<udtCidInit>* sCIDINITS)
    {
        std::string sNodeName;
        bool bExcecute = false;


        for (pugi::xml_node xChildNode : xNodeType.children())
        {

            sNodeName = xChildNode.name();
            bExcecute = bExcecute or (sNodeName == "DOI");
            bExcecute = bExcecute or (sNodeName == "SDI");
            bExcecute = bExcecute or (sNodeName == "DAI");
            bExcecute = bExcecute or (sNodeName == "Val");


            if (bExcecute)
            {
                if (sNodeName == "Val")
                {
                    sCIDINITS->push_back({ LNKey, xChildNode.first_child().value() });
                    return;
                }


                sNodeName = LNKey + "." + xChildNode.attribute("name").as_string();
                AppendInitValue(sNodeName, xChildNode, sCIDINITS);
            }
        }
    }








    // Procedura ricorsiva di ricerca annidata tra le voci DAType
    // 
    // 2025.12.05 - Gestione attributo count nella definizione DOType del fiel CID
    // 2025.12.15 - Esclusione attributi Quality e Timestamp
    //            - Per i nodi con funzione CO aggiunta suffisso .ctlVal senza analizzare il type
    //
    // 2025.12.18 - Esclusione altri attributi per
    //            - FC = CF/EX - cdc=VSG
    // 
    // 2025.12.21 - Determinazione xTimeStamp, xAlwaysUpdate
    //            - Inizializzazione dei SP come da CID
    // 
    // 2026.01.20 - Inizializzazione dei ST / CF / DC da CID
    //
    void AppendDOType(std::string LNKey, pugi::xml_node  xNodeType, pugi::xml_node xDataTemplateNode, std::vector<udtCidData>* sCIDKEYS, std::vector<udtCidInit>* sCIDINITS, std::string sFCPar = "")
    {
        pugi::xml_node xChildTypeNode, xEnumTypeNode;
        std::string LNKeyTyped;
        std::string sNodeName, sTypeName, CountAttrib;
        std::string sFC, sbType, sCDC, sName, sType;
        bool xTimeStamp, xAlwaysUpdate, bIsString;
        int DOTypeCount = 1;

        // Per ogni figlio del nodo passato per parametro
        //
        for (pugi::xml_node xChildNode : xNodeType.children())
        {

            // Estraggo il valore dell'attributo name
            //
            LNKeyTyped = xChildNode.attribute("name").value();
            if (LNKeyTyped.size() == 0)
            {
                // Se non esiste l'attributo name non bisogna proseguire
                return;
            }

            // Controllo la presenza dell'attributo "count"
            //
            DOTypeCount = 1;
            CountAttrib = xChildNode.attribute("count").value();
            if (CountAttrib.size() > 0)
            {
                DOTypeCount = std::stoi(CountAttrib);
            }

            // Eseguo la ricerca in funzione dell'eventuale attributo count
            // 
            // 
            for (int xc = 0; xc < DOTypeCount; xc++)
            {

                // Aggiungo in coda alla chiave LN il tipo - attributo name - 
                // ed eventualmente tra parentesi [] il numero di occorrenza in funzione dell'eventuale attributo count
                //
                LNKeyTyped = LNKey + "." + xChildNode.attribute("name").value();
                if (DOTypeCount > 1)
                {
                    LNKeyTyped = LNKeyTyped + "(" + std::to_string(xc) + ")";
                }

                // Identifico il nome del noto DAType o DOType
                //
                sNodeName = xChildNode.name();
                sNodeName = sNodeName.substr(sNodeName.size() - 2) + "Type";


                // identifico gli attriuti del nodo
                sType = xChildNode.attribute("type").as_string();
                sbType = xChildNode.attribute("bType").as_string();
                sCDC = xChildNode.attribute("cdc").as_string();
                sName = xChildNode.attribute("name").as_string();
                sFC = xChildNode.attribute("fc").as_string();
                if (sFC.size() == 0) { sFC = sFCPar; }


                // se è una funzione di controllo aggiungo direttamente il suffisso .ctlVal
                // ma solo per quelli Oper
                if (sFC == "CO")
                {
                    if (sName == "Oper")
                    {
                        LNKeyTyped = LNKeyTyped + ".ctlVal";

                        // Memorizzo chiave e incremento indice       
                        sCIDKEYS->push_back({ LNKeyTyped, "CO" });
                    }
                }

                // salto i DA con queste funzioni 
//                else if (sFC == "CF" or sFC == "EX")
//                {
//                }
                // salto i DOType con cdc = VSG
                else if (sCDC == "VSG")
                {
                }

                // salto i DA con bType = Quality e Timestamp
                else if (sbType == "Quality" or sbType == "Timestamp")
                {
                }

                //// salto anche tutte le voci con istanza multipla
                //else if (DOTypeCount > 1)
                //{
                //}

                // procedo con l'elaborazione del campo Type
                else
                {
                    // Ricerco il nodo Type 
                    //
                    sTypeName = xChildNode.attribute("type").value();
                    xChildTypeNode = xDataTemplateNode.find_child_by_attribute(sNodeName.c_str(), "id", sTypeName.c_str());
                    if (xChildTypeNode == NULL)
                    { // Se non esiste, la ricerca finisce qui       
                        std::string srctlValue, rValue;
                        bool bNCR = false;

                        // 2026.01.20 
                        //cerco la chiave in sCIDINITS
                        auto xInitVal = std::find_if(sCIDINITS->begin(), sCIDINITS->end(), [&LNKeyTyped](const udtCidInit& item) {return item.sName == LNKeyTyped; });
                        if (xInitVal != sCIDINITS->end())
                        {


                            // se la chiave è un setpoint (funzione SP) allora attivo il flag bNCR
                            //                         
                            bNCR = (sFC == "SP");
                            if (bNCR)
                            {
                                srctlValue = xInitVal->Val;
                            }
                            else // ST - DC
                            {
                                rValue = xInitVal->Val;
                            }



                            // se la chiave è uno stato ricerco il valore del corrispondente Enum
                            if (sFC == "ST" or sFC == "CF")
                            {
                                std::string sEnumTypeName;
                                if (sbType == "Enum")
                                {
                                    xEnumTypeNode = xDataTemplateNode.find_child_by_attribute("EnumType", "id", sType.c_str());
                                    for (pugi::xml_node xEnumNode : xEnumTypeNode.children())
                                    {

                                        if (xEnumNode.first_child().value() == rValue)
                                        {
                                            rValue = xEnumNode.attribute("ord").as_string();
                                            break;
                                        }
                                    }

                                }

                            }
                        }
                        // fine 2026.01.20

                        xTimeStamp = (sFC == "ST" or sFC == "MX");
                        xAlwaysUpdate = (sFC == "ST" or sFC == "MX");
                        bIsString = sbType.find("String") != std::string::npos;

                        // Memorizzo chiave e incremento indice       
                        sCIDKEYS->push_back({ LNKeyTyped, sFC, xTimeStamp, xAlwaysUpdate, srctlValue, bNCR, rValue, bIsString });
                    }
                    else
                    { // se esiste eseguo nuovamente la ricerca in modo ricorsivo            
                        AppendDOType(LNKeyTyped, xChildTypeNode, xDataTemplateNode, sCIDKEYS, sCIDINITS, sFC);
                    }
                }
            }
            // end for

        }
    }





    void GetCidKeysProgram::Execute()
    {
        size_t idx;
        std::vector<udtCidData> sCID_Keys;
        std::vector<udtCidInit> sCID_Inits;


        // Inizializzo il vettore di uscita con quello in ingresso
        //
        int idt = 0;
        for (idx = 0; idx < 999; idx++)
        {
            outCIDKEYS[idx] = inCIDKEYS[idx];
        }



        if (xMode_RQ > 0) {

            // Riporto il modo operativo richiesto con segno negativo
            //
            xMode_Status = -1 * xMode_RQ;



            // Definizione oggetto doc XML_DOCUMENT
            pugi::xml_document doc;

            // Apertura del file CID
            if (doc.load_file(sCIDFilePath.ToString()))
            {
                // il file esiste ed è stato aperto: posso procedere
                //

                // Punto al nodo LDevice nel file CID  
                pugi::xml_node xDevices;
                xDevices = doc.child("SCL").child("IED").child("AccessPoint").child("Server").child("LDevice");

                // Punto al nodo DataType
                pugi::xml_node xDataTypeTemplate;
                xDataTypeTemplate = doc.child("SCL").child("DataTypeTemplates");

                // Definizione variabili di lavoro
                //
                pugi::xml_node LNodeType;
                pugi::xml_node DOType;
                pugi::xml_node DAType;
                pugi::xml_node DASubType;
                std::string LNString;
                std::string TypeString;




                // esegue iterazione per ogni nodo LN per estrarre tutte le inizializzazioni
                //
                for (pugi::xml_node xLN : xDevices.children())
                {
                    // inizializzo la stringa con gli estemi della chiave a monte dei nodi <DOI>
                    LNString = "";
                    LNString = LNString + xLN.attribute("prefix").value();
                    LNString = LNString + xLN.attribute("lnClass").value();
                    LNString = LNString + xLN.attribute("inst").value();

                    AppendInitValue(LNString, xLN, &sCID_Inits);
                }




                // esegue iterazione per ogni nodo LN per estrarre le chiavi CEI presenti nel file CID
                //
                for (pugi::xml_node xLN : xDevices.children())
                {
                    // inizializzo la stringa con gli estemi della chiave a monte dei nodi <DOI>
                    LNString = "";
                    LNString = LNString + xLN.attribute("prefix").value();
                    LNString = LNString + xLN.attribute("lnClass").value();
                    LNString = LNString + xLN.attribute("inst").value();



                    // Recupero il DataTypeTepmplate legata all'attributo lnType
                    LNodeType = xDataTypeTemplate.find_child_by_attribute("id", xLN.attribute("lnType").value());

                    // Richiamo funzione di estrzione delle chiavi dal file CID
                    //
                    AppendDOType(LNString, LNodeType, xDataTypeTemplate, &sCID_Keys, &sCID_Inits);

                }



                // Ordino il vettore delle chiavi
                //
//                std::sort(sGetCidKeys.begin(), sGetCidKeys.end());



                // Aggiorno le chiavi del CID nel vettore di uscita
                //
                int idt = 0;
                for (idx = 0; idx < sCID_Keys.size(); idx++)
                {
                    // trasferisco solo le chiavi che entrano intere nel campo sName
                    //
                    if (sCID_Keys[idx].sName.size() <= _sNAME_MAX_LEN)
                    {
                        outCIDKEYS[idx].sName = sCID_Keys[idx].sName;
                        outCIDKEYS[idx].xTimestamp = sCID_Keys[idx].xTimestamp;
                        outCIDKEYS[idx].xAlwaysUpdate = sCID_Keys[idx].xAlwaysUpdate;

                        // VALORE DI INIZIALIZZAZIONI DEI FC DIVERSI DA SP
                        if (sCID_Keys[idx].sFC != "SP")
                        {
                            if (sCID_Keys[idx].IsString)
                            {
                                outCIDKEYS[idx].sValue = sCID_Keys[idx].rValue;
                            }
                            else
                            {
                                try
                                {
                                    outCIDKEYS[idx].rValue = std::stof(sCID_Keys[idx].rValue);
                                }
                                catch (const std::exception&)
                                {
                                    outCIDKEYS[idx].sValue = sCID_Keys[idx].rValue;
                                }
                            }
                        }

                        // il campo xNDR viene attivato solo con modo operativo richiesto 2
                        //
                        outCIDKEYS[idx].xNDR = sCID_Keys[idx].xNDR and (xMode_RQ == 2);
                        if (outCIDKEYS[idx].xNDR) {
                            // se il valore di inizializzazione è un numervo va in rctrlValue altrimenti va in sValue
                            try
                            {
                                outCIDKEYS[idx].rctlValue = std::stof(sCID_Keys[idx].rctlValue);
                            }
                            catch (const std::exception&)
                            {
                                outCIDKEYS[idx].sValue = sCID_Keys[idx].rctlValue;
                            }
                        }

                        idt++;
                        if (idx >= _61850_ARRAY_SIZE) exit;
                    }
                }
            }

        }

        // Riporto il modo operativo richiesto
        //
        xMode_Status = xMode_RQ;
    }

} // end of namespace GetCidKeys
