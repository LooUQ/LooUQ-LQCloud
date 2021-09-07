
setup()
{

    testStruct = lqc_decomposeSasToken("SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2F864508030074113&sig=bHV12YRyVnKJHtABYi%2BSGlnkvBCpRD0rb7Ak9rg2fxM%3D&se=2872679808");
    char sasString[200] = {0};
    lqc_composeSasToken(sasString, testStruct.url, sasStruct.dvcId, sasStruct.sasSig);

    if (strcmp(sasString, "SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2F864508030074113&sig=bHV12YRyVnKJHtABYi%2BSGlnkvBCpRD0rb7Ak9rg2fxM%3D&se=2872679808") == 0) {
        PRINTF(DBGCOLOR_green, "SAS conversion successful.\r"); }
    else {
        PRINTF(DBGCOLOR_red, "SAS conversion FAILED.\r"); }    
}


loop()
{
}