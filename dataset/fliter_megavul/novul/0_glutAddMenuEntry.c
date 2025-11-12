void FGAPIENTRY glutAddMenuEntry( const char* label, int value )
{
    SFG_MenuEntry* menuEntry;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutAddMenuEntry" );

    freeglut_return_if_fail( fgStructure.CurrentMenu );
    if (fgState.ActiveMenus)
        fgError("Menu manipulation not allowed while menus in use.");

    menuEntry = (SFG_MenuEntry *)calloc( sizeof(SFG_MenuEntry), 1 );
    menuEntry->Text = strdup( label );
    menuEntry->ID   = value;

    /* Have the new menu entry attached to the current menu */
    fgListAppend( &fgStructure.CurrentMenu->Entries, &menuEntry->Node );

    fghCalculateMenuBoxSize( );
}