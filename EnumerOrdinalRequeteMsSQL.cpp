// EnumerOrdinalRequeteMsSQL.cpp : Définit le point d'entrée de l'application.
//

#define WIN32_LEAN_AND_MEAN  
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <SDKDDKVer.h>
#include <sql.h> 
#include <sqlext.h> 
#include <iostream>
#include <string>
#include <atlbase.h>
#include <chrono>
#include <comdef.h>
#include <commctrl.h>
#include <commdlg.h>
#include "EnumerOrdinalRequeteMsSQL.h"
#pragma comment(lib, "Comctl32")
#pragma warning(disable : 4996)
#pragma warning(disable:2011)
#pragma warning(disable:4005)

#define MAX_LOADSTRING 100

// Variables globales :
HINSTANCE hInst;                                // instance actuelle
CHAR szTitle[MAX_LOADSTRING];                  // Texte de la barre de titre
INITCOMMONCONTROLSEX icex;
WNDCLASSEX wcex;
SYSTEMTIME st;
HWND hMain;
HICON icone;
HBRUSH Fond;
CHAR Serveur[0x50];
CHAR Catalogue[0x32];
CHAR computerName[0x10];
SQLHANDLE sqlEnvHandle;
SQLHANDLE sqlConnHandle;
SQLHANDLE sqlStatementHandle;
SQLRETURN retCode = 0;
SQLCHAR message[1024];
SQLCHAR SQLState[1024];

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int MsgBox(char* lpszText, char* lpszCaption, DWORD dwStyle, int lpszIcon) {
    MSGBOXPARAMS lpmbp{};
    lpmbp.hInstance = wcex.hInstance;
    lpmbp.cbSize = sizeof(MSGBOXPARAMS);
    lpmbp.hwndOwner = hMain;
    lpmbp.dwLanguageId = MAKELANGID(0x0800, 0x0800); //par defaut celui du systeme
    lpmbp.lpszText = lpszText;
    lpmbp.lpszCaption = lpszCaption;
    lpmbp.dwStyle = dwStyle | MB_DEFBUTTON1 | MB_TASKMODAL | MB_SETFOREGROUND | MB_RTLREADING;
    lpmbp.lpszIcon = (LPCTSTR)lpszIcon;
    lpmbp.lpfnMsgBoxCallback = 0;
    return  MessageBoxIndirect(&lpmbp);
}
int MsgBox(const char* lpszText, const char* lpszCaption, DWORD dwStyle, int lpszIcon) {
    MSGBOXPARAMS lpmbp{};
    lpmbp.hInstance = wcex.hInstance;
    lpmbp.cbSize = sizeof(MSGBOXPARAMS);
    lpmbp.hwndOwner = hMain;
    lpmbp.dwLanguageId = MAKELANGID(0x0800, 0x0800);
    lpmbp.lpszText = lpszText;
    lpmbp.lpszCaption = lpszCaption;
    lpmbp.dwStyle = dwStyle | MB_DEFBUTTON1 | MB_SETFOREGROUND | MB_RTLREADING | MB_SERVICE_NOTIFICATION;
    lpmbp.lpszIcon = (LPCTSTR)lpszIcon;
    lpmbp.lpfnMsgBoxCallback = 0;
    return  MessageBoxIndirect(&lpmbp);
}
void AfficherErreur(unsigned int handletype, const SQLHANDLE& handle) {
    if (SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, SQLState, NULL, message, 1024, NULL))
    {
        CHAR buff[0x200];
        wsprintf(buff, "Information: %sEtatSQL: %s", message, SQLState);
        MsgBox(buff, nullptr, MB_OK, 142);
    }
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPWSTR    lpCmdLine,_In_ int       nCmdShow){
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    hMain = GetDesktopWindow();
    std::locale::global(std::locale("fr_CA.UTF-8"));
    std::cout.imbue(std::locale());
    GetLocalTime(&st);
    hMain = GetDesktopWindow();
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    icone = LoadIcon(hInstance, MAKEINTRESOURCE(0x8D));
    Fond = (HBRUSH)(COLOR_WINDOW + 1);
    hInst = hInstance;
    return (int)DialogBox(wcex.hInstance, MAKEINTRESOURCE(0x69), hMain, WndProc);
}
// Gestionnaire de messages pour la boîte de dialogue À propos de.
INT_PTR CALLBACK ConnexionSQL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
INT_PTR CALLBACK WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    hMain = hDlg;
    switch (message)
    {
    case WM_INITDIALOG:
    {
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);
        DWORD size = sizeof(computerName) / sizeof(computerName[0]);
        if (GetComputerNameA(computerName, &size))
        {
            wsprintf(Serveur, "%s\\SQLEXPRESS", computerName);
            SetDlgItemText(hDlg, 0x1770, computerName);
            if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))              return -1;
            if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0))  return -1;
            if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle))                return -1;
            SQLCHAR retConString[1024];
            CHAR constr[MAX_PATH];
            SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)icone);
            int rs=DialogBox(hInst,(LPSTR)0x6A , hDlg, ConnexionSQL);
            wsprintf(constr, "DRIVER={SQL Server};SERVER=%s;DATABASE=Master;UID=sa;PWD=Password01$;", Serveur );
            switch (SQLDriverConnect(sqlConnHandle, NULL, (SQLCHAR*)constr, SQL_NTS, retConString, 1024, NULL, SQL_DRIVER_NOPROMPT))
            {
                case SQL_SUCCESS_WITH_INFO:     AfficherErreur(SQL_HANDLE_DBC, sqlConnHandle);        break;
                case SQL_INVALID_HANDLE:
                case SQL_ERROR:                 AfficherErreur(SQL_HANDLE_DBC, sqlConnHandle);        return -1;
                default:        break;
            }
            wsprintf(szTitle, "SA est connecte au serveur: %s", Serveur);
            SetDlgItemText(hDlg, 0x1770, szTitle);
        }
        else {
            MsgBox((CHAR*)GetLastError(), (char*)"Erreur d'aquisition nom PC", MB_ICONHAND | MB_APPLMODAL | MB_OK, 0x8E);
        }
    }
    return (INT_PTR)TRUE;

    case WM_COMMAND:{
            switch (LOWORD(wParam))
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hDlg);
                break;
            default:
                return DefWindowProc(hDlg, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hDlg, &ps);
            // TODO: Ajoutez ici le code de dessin qui utilise hdc...
            EndPaint(hDlg, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hDlg, message, wParam, lParam);
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

/*
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <string>

void print_error(SQLHANDLE handle, SQLSMALLINT type) {
    SQLCHAR sqlState[1024];
    SQLCHAR message[1024];
    if (SQLGetDiagRec(type, handle, 1, sqlState, NULL, message, 1024, NULL) == SQL_SUCCESS) {
        std::cerr << "SQL Error [" << sqlState << "]: " << message << std::endl;
    }
}

int main() {

    // Allocation des handles
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))
        return -1;
    if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0))
        return -1;
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnectionHandle))
        return -1;

    // Connexion à la base de données
    SQLCHAR retConString[1024];
    if (SQL_SUCCESS != SQLDriverConnect(sqlConnectionHandle, NULL,
            (SQLCHAR*)"DRIVER={SQL Server};SERVER=YOUR_SERVER_NAME;DATABASE=YOUR_DATABASE_NAME;UID=YOUR_USERNAME;PWD=YOUR_PASSWORD;",
            SQL_NTS, retConString, 1024, NULL, SQL_DRIVER_NOPROMPT)) {
        print_error(sqlConnectionHandle, SQL_HANDLE_DBC);
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnectionHandle);
        return -1;
    }

    // Allocation du handle de la déclaration
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnectionHandle, &sqlStatementHandle))
        return -1;

    // Exécution de la requête
    if (SQL_SUCCESS != SQLExecDirect(sqlStatementHandle, (SQLCHAR*)"SELECT * FROM YOUR_TABLE_NAME", SQL_NTS)) {
        print_error(sqlStatementHandle, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStatementHandle);
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnectionHandle);
        return -1;
    }

    // Récupération des ordinals des colonnes
    SQLSMALLINT numColumns;
    SQLNumResultCols(sqlStatementHandle, &numColumns);

    std::cout << "Ordinals des colonnes dans la table:" << std::endl;
    for (SQLSMALLINT i = 1; i <= numColumns; ++i) {
        SQLCHAR columnName[256];
        SQLSMALLINT nameLength;
        SQLDescribeCol(sqlStatementHandle, i, columnName, sizeof(columnName), &nameLength, NULL, NULL, NULL, NULL);
        std::cout << "Colonne " << i << ": " << columnName << std::endl;
    }

    // Libération des handles
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStatementHandle);
    SQLDisconnect(sqlConnectionHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnectionHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);

    return 0;
}
*/