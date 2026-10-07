Attribute VB_Name = "MiniSumoChasis"
'==============================================================================
' MiniSumo UVM 2026 - Chasis Rev. B (traccion trasera)
' Construye el chasis con arbol parametrico y ecuaciones globales.
'
' USO:  SolidWorks > Herramientas > Macro > Nueva...  (guarda un .swp vacio)
'       En el editor VBA:  Archivo > Importar archivo...  y elige este .bas
'       Selecciona el modulo MiniSumoChasis y pulsa F5
'
' NOTA: la API de SolidWorks trabaja en METROS. Las constantes de abajo estan
'       en milimetros y se convierten con MM().
'==============================================================================
Option Explicit

Dim swApp       As Object
Dim swModel     As Object
Dim swPart      As Object
Dim swFeatMgr   As Object
Dim swSketchMgr As Object
Dim swSelMgr    As Object
Dim swEqnMgr    As Object
Dim boolstatus  As Boolean

'--------------------------- parametros maestros ------------------------------
Const P_ANCHO   As Double = 100#    ' envolvente X
Const P_PROF    As Double = 100#    ' envolvente Y
Const P_ALT     As Double = 24#     ' altura de la pieza
Const P_BASE_T  As Double = 3#      ' espesor de la placa base
Const P_PARED   As Double = 2.4     ' espesor de pared (6 lineas de 0.4)
Const P_EJE_Y   As Double = 72#     ' eje motriz desde el frente
Const P_EJE_Z   As Double = 13.5    ' altura del eje en coords de la pieza
Const P_RUEDA_D As Double = 35#
Const P_RUEDA_W As Double = 12#
Const P_HOLG_R  As Double = 1#      ' holgura lateral de rueda
Const P_MOT_W   As Double = 10#     ' ancho del cuerpo N20
Const P_MOT_H   As Double = 12#     ' alto del cuerpo N20
Const P_MOT_L   As Double = 26#     ' largo del cuerpo N20
Const P_AJUSTE  As Double = 0.4     ' holgura de impresion
Const P_BAT_X   As Double = 55#
Const P_BAT_Y   As Double = 30#
Const P_BAT_Z   As Double = 13#
Const P_BAT_Y0  As Double = 26#     ' inicio del bolsillo de bateria
Const P_CUNA_Y  As Double = 6#      ' espesor del bloque de montaje
Const P_TOR_H   As Double = 22#     ' altura de la torre de sensores
Const P_TOR_T   As Double = 4#      ' espesor de la torre
Const P_NERV_X  As Double = 17#     ' posicion de los nervios

Function MM(v As Double) As Double
    MM = v / 1000#
End Function

'------------------------- helpers de seleccion -------------------------------
' Los nombres de los planos cambian con el idioma de la instalacion.
Function SelPlano(cual As String) As Boolean
    Dim nombres As Variant
    Dim i       As Integer
    Select Case cual
        Case "TOP":   nombres = Array("Top Plane", "Planta", "Vista superior")
        Case "FRONT": nombres = Array("Front Plane", "Alzado", "Vista frontal")
        Case "RIGHT": nombres = Array("Right Plane", "Vista lateral", "Derecha")
    End Select
    For i = 0 To UBound(nombres)
        swModel.ClearSelection2 True
        If swModel.Extension.SelectByID2(CStr(nombres(i)), "PLANE", 0, 0, 0, _
                                          False, 0, Nothing, 0) Then
            SelPlano = True
            Exit Function
        End If
    Next i
    SelPlano = False
End Function

' Croquiza un rectangulo por esquinas, en mm, sobre el croquis activo
Sub Rect(x1 As Double, y1 As Double, x2 As Double, y2 As Double)
    swSketchMgr.CreateCornerRectangle MM(x1), MM(y1), 0, MM(x2), MM(y2), 0
End Sub

Sub Circ(cx As Double, cy As Double, d As Double)
    swSketchMgr.CreateCircleByRadius MM(cx), MM(cy), 0, MM(d / 2#)
End Sub

' Extrusion ciega desde el plano del croquis
Sub Extruir(prof As Double, desde As Double)
    swFeatMgr.FeatureExtrusion3 True, False, False, 0, 0, MM(prof), 0, _
        False, False, False, False, 0, 0, False, False, False, False, _
        True, True, True, 0, 0, False
    If desde <> 0 Then
        ' el desplazamiento se resuelve con un plano de referencia previo
    End If
End Sub

'================================= MAIN =======================================
Sub main()

    Set swApp = Application.SldWorks
    Set swModel = swApp.NewPart()
    If swModel Is Nothing Then
        MsgBox "No se pudo crear la pieza. Revisa la plantilla por defecto."
        Exit Sub
    End If
    Set swPart = swModel
    Set swFeatMgr = swModel.FeatureManager
    Set swSketchMgr = swModel.SketchManager
    Set swSelMgr = swModel.SelectionManager
    Set swEqnMgr = swModel.GetEquationMgr

    swModel.SetUserPreferenceIntegerValue 71, 0   ' unidades: MMGS

    '--- 0. ecuaciones globales ------------------------------------------------
    AgregarEcuaciones

    '--- 1. placa base ---------------------------------------------------------
    If Not SelPlano("TOP") Then
        MsgBox "No se encontro el plano de planta. Comprueba el idioma de SolidWorks."
        Exit Sub
    End If
    swSketchMgr.InsertSketch True
    Rect 0, 0, P_ANCHO, P_PROF
    swSketchMgr.InsertSketch True
    swFeatMgr.FeatureExtrusion3 True, False, False, 0, 0, MM(P_BASE_T), 0, _
        False, False, False, False, 0, 0, False, False, False, False, _
        True, True, True, 0, 0, False
    RenombrarUltima "PlacaBase"

    '--- 2. paredes perimetrales ----------------------------------------------
    Dim planoSup As Object
    Set planoSup = CrearPlanoOffset(P_BASE_T, "PL_SobreBase")
    swModel.ClearSelection2 True
    boolstatus = swModel.Extension.SelectByID2("PL_SobreBase", "PLANE", 0, 0, 0, _
                                               False, 0, Nothing, 0)
    swSketchMgr.InsertSketch True
    Rect 0, 0, P_ANCHO, P_PROF
    Rect P_PARED, P_PARED, P_ANCHO - P_PARED, P_PROF - P_PARED
    swSketchMgr.InsertSketch True
    swFeatMgr.FeatureExtrusion3 True, False, False, 0, 0, MM(P_ALT - P_BASE_T), 0, _
        False, False, False, False, 0, 0, False, False, False, False, _
        True, True, True, 0, 0, False
    RenombrarUltima "Paredes"

    '--- 3. recortes de rueda (pasantes) --------------------------------------
    Dim recW As Double, recL As Double
    recW = P_RUEDA_W + 2 * P_HOLG_R
    recL = P_RUEDA_D + 3#
    If Not SelPlano("TOP") Then Exit Sub
    swSketchMgr.InsertSketch True
    Rect 0, P_EJE_Y - recL / 2, recW, P_EJE_Y + recL / 2
    Rect P_ANCHO - recW, P_EJE_Y - recL / 2, P_ANCHO, P_EJE_Y + recL / 2
    swSketchMgr.InsertSketch True
    CortarPasante
    RenombrarUltima "RecortesRueda"

    '--- 4. alojamientos de motor ---------------------------------------------
    Dim zMot As Double
    zMot = P_EJE_Z - (P_MOT_H + P_AJUSTE) / 2#
    Call CrearPlanoOffset(zMot, "PL_Motores")
    swModel.ClearSelection2 True
    boolstatus = swModel.Extension.SelectByID2("PL_Motores", "PLANE", 0, 0, 0, _
                                               False, 0, Nothing, 0)
    swSketchMgr.InsertSketch True
    Dim mw As Double
    mw = P_MOT_W + P_AJUSTE
    Rect recW, P_EJE_Y - mw / 2, recW + P_MOT_L + P_AJUSTE, P_EJE_Y + mw / 2
    Rect P_ANCHO - recW - P_MOT_L - P_AJUSTE, P_EJE_Y - mw / 2, _
         P_ANCHO - recW, P_EJE_Y + mw / 2
    swSketchMgr.InsertSketch True
    CortarCiego P_MOT_H + P_AJUSTE
    RenombrarUltima "AlojamientosMotor"

    '--- 5. bolsillo de bateria -----------------------------------------------
    Call CrearPlanoOffset(P_ALT, "PL_Superior")
    swModel.ClearSelection2 True
    boolstatus = swModel.Extension.SelectByID2("PL_Superior", "PLANE", 0, 0, 0, _
                                               False, 0, Nothing, 0)
    swSketchMgr.InsertSketch True
    Rect (P_ANCHO - P_BAT_X - 1#) / 2, P_BAT_Y0, _
         (P_ANCHO - P_BAT_X - 1#) / 2 + P_BAT_X + 1#, P_BAT_Y0 + P_BAT_Y + 1#
    swSketchMgr.InsertSketch True
    CortarCiegoInverso P_ALT - P_BASE_T
    RenombrarUltima "BolsilloBateria"

    '--- 6. nervios longitudinales --------------------------------------------
    swModel.ClearSelection2 True
    boolstatus = swModel.Extension.SelectByID2("PL_SobreBase", "PLANE", 0, 0, 0, _
                                               False, 0, Nothing, 0)
    swSketchMgr.InsertSketch True
    Rect P_NERV_X, P_PARED, P_NERV_X + P_PARED, P_PROF - P_PARED
    Rect P_ANCHO - P_NERV_X - P_PARED, P_PARED, P_ANCHO - P_NERV_X, P_PROF - P_PARED
    swSketchMgr.InsertSketch True
    swFeatMgr.FeatureExtrusion3 True, False, False, 0, 0, MM(9#), 0, _
        False, False, False, False, 0, 0, False, False, False, False, _
        True, True, True, 0, 0, False
    RenombrarUltima "NerviosRigidez"

    '--- 7. bloque frontal para la cuna ---------------------------------------
    swModel.ClearSelection2 True
    boolstatus = swModel.Extension.SelectByID2("PL_SobreBase", "PLANE", 0, 0, 0, _
                                               False, 0, Nothing, 0)
    swSketchMgr.InsertSketch True
    Rect P_PARED, P_PARED, P_ANCHO - P_PARED, P_PARED + P_CUNA_Y
    swSketchMgr.InsertSketch True
    swFeatMgr.FeatureExtrusion3 True, False, False, 0, 0, MM(10#), 0, _
        False, False, False, False, 0, 0, False, False, False, False, _
        True, True, True, 0, 0, False
    RenombrarUltima "BloqueCuna"

    '--- 8. torre de sensores -------------------------------------------------
    swModel.ClearSelection2 True
    boolstatus = swModel.Extension.SelectByID2("PL_SobreBase", "PLANE", 0, 0, 0, _
                                               False, 0, Nothing, 0)
    swSketchMgr.InsertSketch True
    Rect P_ANCHO / 2 - 22#, P_PARED + P_CUNA_Y, _
         P_ANCHO / 2 + 22#, P_PARED + P_CUNA_Y + P_TOR_T
    swSketchMgr.InsertSketch True
    swFeatMgr.FeatureExtrusion3 True, False, False, 0, 0, MM(P_TOR_H), 0, _
        False, False, False, False, 0, 0, False, False, False, False, _
        True, True, True, 0, 0, False
    RenombrarUltima "TorreSensores"

    '--- 9. ventanas de sensores de linea -------------------------------------
    If Not SelPlano("TOP") Then Exit Sub
    swSketchMgr.InsertSketch True
    Circ 26#, 24#, 10#
    Circ 74#, 24#, 10#
    Circ 26#, 94#, 10#
    Circ 74#, 94#, 10#
    swSketchMgr.InsertSketch True
    CortarCiego P_BASE_T
    RenombrarUltima "VentanasSensoresLinea"

    '--- 10. anclajes de lastre y patines -------------------------------------
    If Not SelPlano("TOP") Then Exit Sub
    swSketchMgr.InsertSketch True
    Circ 40#, 62#, 4.2
    Circ 60#, 62#, 4.2
    Circ 40#, 82#, 4.2
    Circ 60#, 82#, 4.2
    Circ 12#, 16#, 4.2
    Circ P_ANCHO - 12#, 16#, 4.2
    swSketchMgr.InsertSketch True
    CortarCiego 9#
    RenombrarUltima "AnclajesInsertos"

    swModel.ViewZoomtofit2
    swModel.ClearSelection2 True
    swModel.EditRebuild3

    MsgBox "Chasis MiniSumo Rev. B generado." & vbCrLf & vbCrLf & _
           "Las cotas estan en las ecuaciones globales:" & vbCrLf & _
           "Herramientas > Ecuaciones." & vbCrLf & vbCrLf & _
           "Pendiente de anadir a mano: escuadras de la torre, " & _
           "postes de la placa de control y ranuras de la cuna."

End Sub

'=========================== rutinas de apoyo =================================

Sub AgregarEcuaciones()
    On Error Resume Next
    swEqnMgr.Add2 -1, """ANCHO"" = " & P_ANCHO, True
    swEqnMgr.Add2 -1, """PROF"" = " & P_PROF, True
    swEqnMgr.Add2 -1, """ALT"" = " & P_ALT, True
    swEqnMgr.Add2 -1, """BASE_T"" = " & P_BASE_T, True
    swEqnMgr.Add2 -1, """PARED"" = " & P_PARED, True
    swEqnMgr.Add2 -1, """EJE_Y"" = " & P_EJE_Y, True
    swEqnMgr.Add2 -1, """EJE_Z"" = " & P_EJE_Z, True
    swEqnMgr.Add2 -1, """RUEDA_D"" = " & P_RUEDA_D, True
    swEqnMgr.Add2 -1, """RUEDA_W"" = " & P_RUEDA_W, True
    swEqnMgr.Add2 -1, """MOT_L"" = " & P_MOT_L, True
    swEqnMgr.Add2 -1, """AJUSTE"" = " & P_AJUSTE, True
    On Error GoTo 0
End Sub

Function CrearPlanoOffset(dist As Double, nombre As String) As Object
    On Error Resume Next
    If Not SelPlano("TOP") Then Exit Function
    Dim swFeat As Object
    Set swFeat = swFeatMgr.InsertRefPlane(8, MM(dist), 0, 0, 0, 0)
    If Not swFeat Is Nothing Then swFeat.Name = nombre
    Set CrearPlanoOffset = swFeat
    swModel.ClearSelection2 True
    On Error GoTo 0
End Function

Sub CortarPasante()
    swFeatMgr.FeatureCut4 True, False, False, 1, 1, 0.01, 0.01, _
        False, False, False, False, 0, 0, False, False, False, False, False, _
        True, True, True, True, False, 0, 0, False, False
End Sub

Sub CortarCiego(prof As Double)
    swFeatMgr.FeatureCut4 True, False, False, 0, 0, MM(prof), 0, _
        False, False, False, False, 0, 0, False, False, False, False, False, _
        True, True, True, True, False, 0, 0, False, False
End Sub

Sub CortarCiegoInverso(prof As Double)
    swFeatMgr.FeatureCut4 True, False, True, 0, 0, MM(prof), 0, _
        False, False, False, False, 0, 0, False, False, False, False, False, _
        True, True, True, True, False, 0, 0, False, False
End Sub

Sub RenombrarUltima(nombre As String)
    On Error Resume Next
    Dim swFeat As Object
    Set swFeat = swModel.FeatureByPositionReverse(0)
    If Not swFeat Is Nothing Then swFeat.Name = nombre
    On Error GoTo 0
End Sub
