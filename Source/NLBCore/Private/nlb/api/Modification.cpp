#include "nlb/api/Modification.h"

std::string Modification::typeToString(Type type) {
    switch (type) {
        case Type::ASSIGN:
            return "ASSIGN";
        case Type::TAG:
            return "TAG";
        case Type::GETTAG:
            return "GETTAG";
        case Type::WHILE:
            return "WHILE";
        case Type::IF:
            return "IF";
        case Type::IFHAVE:
            return "IFHAVE";
        case Type::ELSE:
            return "ELSE";
        case Type::ELSEIF:
            return "ELSEIF";
        case Type::END:
            return "END";
        case Type::RETURN:
            return "RETURN";
        case Type::HAVE:
            return "HAVE";
        case Type::CLONE:
            return "CLONE";
        case Type::CNTNR:
            return "CNTNR";
        case Type::ID:
            return "ID";
        case Type::ADD:
            return "ADD";
        case Type::ADDU:
            return "ADDU";
        case Type::ADDINV:
            return "ADDINV";
        case Type::ADDALL:
            return "ADDALL";
        case Type::ADDALLU:
            return "ADDALLU";
        case Type::REMOVE:
            return "REMOVE";
        case Type::RMINV:
            return "RMINV";
        case Type::CLEAR:
            return "CLEAR";
        case Type::CLRINV:
            return "CLRINV";
        case Type::OBJS:
            return "OBJS";
        case Type::SSND:
            return "SSND";
        case Type::WSND:
            return "WSND";
        case Type::SND:
            return "SND";
        case Type::SPUSH:
            return "SPUSH";
        case Type::WPUSH:
            return "WPUSH";
        case Type::PUSH:
            return "PUSH";
        case Type::POP:
            return "POP";
        case Type::SINJECT:
            return "SINJECT";
        case Type::INJECT:
            return "INJECT";
        case Type::EJECT:
            return "EJECT";
        case Type::SHUFFLE:
            return "SHUFFLE";
        case Type::PRN:
            return "PRN";
        case Type::DSC:
            return "DSC";
        case Type::PDSC:
            return "PDSC";
        case Type::PDSCS:
            return "PDSCS";
        case Type::ACT:
            return "ACT";
        case Type::ACTT:
            return "ACTT";
        case Type::ACTF:
            return "ACTF";
        case Type::USE:
            return "USE";
        case Type::SIZE:
            return "SIZE";
        case Type::RND:
            return "RND";
        case Type::ACHMAX:
            return "ACHMAX";
        case Type::ACHIEVE:
            return "ACHIEVE";
        case Type::ACHIEVED:
            return "ACHIEVED";
        case Type::GOTO:
            return "GOTO";
        case Type::SNAPSHOT:
            return "SNAPSHOT";
        case Type::COUNTGET:
            return "COUNTGET";
        case Type::COUNTRST:
            return "COUNTRST";
        case Type::OPENURL:
            return "OPENURL";
        case Type::WINGEOM:
            return "WINGEOM";
        case Type::INVGEOM:
            return "INVGEOM";
        case Type::WINCOLOR:
            return "WINCOLOR";
        case Type::INVCOLOR:
            return "INVCOLOR";
        default:
            return "ASSIGN";
    }
}

Modification::Type Modification::stringToType(const std::string& typeStr) {
    if (typeStr == "TAG") return Type::TAG;
    if (typeStr == "GETTAG") return Type::GETTAG;
    if (typeStr == "WHILE") return Type::WHILE;
    if (typeStr == "IF") return Type::IF;
    if (typeStr == "IFHAVE") return Type::IFHAVE;
    if (typeStr == "ELSE") return Type::ELSE;
    if (typeStr == "ELSEIF") return Type::ELSEIF;
    if (typeStr == "END") return Type::END;
    if (typeStr == "RETURN") return Type::RETURN;
    if (typeStr == "HAVE") return Type::HAVE;
    if (typeStr == "CLONE") return Type::CLONE;
    if (typeStr == "CNTNR") return Type::CNTNR;
    if (typeStr == "ID") return Type::ID;
    if (typeStr == "ADD") return Type::ADD;
    if (typeStr == "ADDU") return Type::ADDU;
    if (typeStr == "ADDINV") return Type::ADDINV;
    if (typeStr == "ADDALL") return Type::ADDALL;
    if (typeStr == "ADDALLU") return Type::ADDALLU;
    if (typeStr == "REMOVE") return Type::REMOVE;
    if (typeStr == "RMINV") return Type::RMINV;
    if (typeStr == "CLEAR") return Type::CLEAR;
    if (typeStr == "CLRINV") return Type::CLRINV;
    if (typeStr == "OBJS") return Type::OBJS;
    if (typeStr == "SSND") return Type::SSND;
    if (typeStr == "WSND") return Type::WSND;
    if (typeStr == "SND") return Type::SND;
    if (typeStr == "SPUSH") return Type::SPUSH;
    if (typeStr == "WPUSH") return Type::WPUSH;
    if (typeStr == "PUSH") return Type::PUSH;
    if (typeStr == "POP") return Type::POP;
    if (typeStr == "SINJECT") return Type::SINJECT;
    if (typeStr == "INJECT") return Type::INJECT;
    if (typeStr == "EJECT") return Type::EJECT;
    if (typeStr == "SHUFFLE") return Type::SHUFFLE;
    if (typeStr == "PRN") return Type::PRN;
    if (typeStr == "DSC") return Type::DSC;
    if (typeStr == "PDSC") return Type::PDSC;
    if (typeStr == "PDSCS") return Type::PDSCS;
    if (typeStr == "ACT") return Type::ACT;
    if (typeStr == "ACTT") return Type::ACTT;
    if (typeStr == "ACTF") return Type::ACTF;
    if (typeStr == "USE") return Type::USE;
    if (typeStr == "SIZE") return Type::SIZE;
    if (typeStr == "RND") return Type::RND;
    if (typeStr == "ACHMAX") return Type::ACHMAX;
    if (typeStr == "ACHIEVE") return Type::ACHIEVE;
    if (typeStr == "ACHIEVED") return Type::ACHIEVED;
    if (typeStr == "GOTO") return Type::GOTO;
    if (typeStr == "SNAPSHOT") return Type::SNAPSHOT;
    if (typeStr == "COUNTGET") return Type::COUNTGET;
    if (typeStr == "COUNTRST") return Type::COUNTRST;
    if (typeStr == "OPENURL") return Type::OPENURL;
    if (typeStr == "WINGEOM") return Type::WINGEOM;
    if (typeStr == "INVGEOM") return Type::INVGEOM;
    if (typeStr == "WINCOLOR") return Type::WINCOLOR;
    if (typeStr == "INVCOLOR") return Type::INVCOLOR;
    
    // Default
    return Type::ASSIGN;
}