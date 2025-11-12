void FGAPIENTRY glutAddSubMenu( const char *label, int subMenuID )
{
    SFG_MenuEntry *menuEntry;
    SFG_Menu *subMenu;

    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutAddSubMenu" );
    subMenu = fgMenuByID( subMenuID );

    freeglut_return_if_fail( fgStructure.CurrentMenu );
    if (fgState.ActiveMenus)
        fgError("Menu manipulation not allowed while menus in use.");

    freeglut_return_if_fail( subMenu );

    menuEntry = ( SFG_MenuEntry * )calloc( sizeof( SFG_MenuEntry ), 1 );
    menuEntry->Text    = strdup( label );
    menuEntry->SubMenu = subMenu;
    menuEntry->ID      = -1;

    fgListAppend( &fgStructure.CurrentMenu->Entries, &menuEntry->Node );
    fghCalculateMenuBoxSize( );
}