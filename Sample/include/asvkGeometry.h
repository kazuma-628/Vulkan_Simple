﻿//-------------------------------------------------------------------------------------------------
// File : asvkGeometry.h
// Desc : Geometry Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------
#pragma once

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include <asvkTypedef.h>
#include <asvkMath.h>
#include <array>


namespace asvk {

//-------------------------------------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------------------------------------
struct Ray;
struct BoundingBox;
struct BoundingSphere;
class  Frustum;


///////////////////////////////////////////////////////////////////////////////////////////////////
// Ray structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct Ray
{
    Vector3 pos;        //!< 位置座標です.
    Vector3 dir;        //!< 方向ベクトルです.
    Vector3 invDir;     //!< 方向ベクトルの逆数です.
    int3    sign;       //!< 方向ベクトルの符号です.

    //---------------------------------------------------------------------------------------------
    //! @brief      引き付きコンストラクタです.
    //!
    //! @param[in]      poisition       位置座標です.
    //! @param[in]      direction       方向ベクトルです.
    //---------------------------------------------------------------------------------------------
    Ray( const Vector3& position, const Vector3& direction );

    //---------------------------------------------------------------------------------------------
    //! @brief      コピーコンストラクタです.
    //!
    //! @param[in]      value       コピー元の値です.
    //---------------------------------------------------------------------------------------------
    Ray( const Ray& value );

    //---------------------------------------------------------------------------------------------
    //! @brief      レイを更新します.
    //!
    //! @param[in]      position        位置座標です.
    //! @param[in]      direction       方向ベクトルです.
    //---------------------------------------------------------------------------------------------
    void Update( const Vector3& position, const Vector3& direction );
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// BoundingBox structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BoundingBox
{
    Vector3   mini;       //!< 最小値です.
    Vector3   maxi;       //!< 最大値です.

    //---------------------------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //---------------------------------------------------------------------------------------------
    BoundingBox();

    //---------------------------------------------------------------------------------------------
    //! @brief      引数付きコンストラクタです.
    //!
    //! @param[in]      _mini       最小値です.
    //! @param[in]      _maxi       最大値です.
    //---------------------------------------------------------------------------------------------
    BoundingBox( const Vector3& _mini, const Vector3& _maxi );

    //---------------------------------------------------------------------------------------------
    //! @brief      コピーコンストラクタです.
    //!
    //! @param[in]      value       コピー元の値です.
    //---------------------------------------------------------------------------------------------
    BoundingBox( const BoundingBox& value );

    //---------------------------------------------------------------------------------------------
    //! @brief      中心座標を取得します.
    //!
    //! @return     中心座標を返却します.
    //---------------------------------------------------------------------------------------------
    Vector3 GetCenter() const;

    //---------------------------------------------------------------------------------------------
    //! @brief      8角の頂点を取得します.
    //!
    //! @return     8角の頂点を返却します.
    //---------------------------------------------------------------------------------------------
    std::array<Vector3, 8>  GetCorners() const;

    //---------------------------------------------------------------------------------------------
    //! @brief      点を含むか判定します.
    //!
    //! @param[in]      point       判定する点です.
    //! @retval true    ボックス内です.
    //! @retval false   ボックス外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const Vector3& point ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      凸包を含むか判定します.
    //!
    //! @param[in]      pVertices       頂点配列.
    //! @param[in]      count           頂点数.
    //! @retval true    ボックス内です.
    //! @retval false   ボックス外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const Vector3* pVertices, const u32 count ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      バウンディングスフィアを含むか判定します.
    //!
    //! @param[in]      sphere      判定するバウンディングスフィアです.
    //! @retval true    ボックス内です.
    //! @retval false   ボックス外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const BoundingSphere& sphere ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      バウンディングボックスを含むか判定します.
    //!
    //! @param[in]      box         判定するバウンディングボックスです.
    //! @retval true    ボックス内です.
    //! @retval false   ボックス外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const BoundingBox& box ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      マージします.
    //!
    //! @param[in]      value       マージする点.
    //---------------------------------------------------------------------------------------------
    void Merge( const Vector3& value );

    //---------------------------------------------------------------------------------------------
    //! @brief      マージします.
    //!
    //! @param[in]      a           マージするバウンディングボックス.
    //! @param[in]      b           マージするバウンディングボックス.
    //! @return     2つのバウンディングボックスをマージした結果を返却します.
    //---------------------------------------------------------------------------------------------
    static BoundingBox Merge( const BoundingBox& a, const BoundingBox& b );
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// BoundingSpherer structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BoundingSphere
{
    Vector3   center;     //!< 中心座標です.
    float       radius;     //!< 半径です.

    //---------------------------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //---------------------------------------------------------------------------------------------
    BoundingSphere();

    //---------------------------------------------------------------------------------------------
    //! @brief      引数付きコンストラクタです.
    //!
    //! @param[in]      _center     中心座標です.
    //! @param[in]      _radius     半径です.
    //---------------------------------------------------------------------------------------------
    BoundingSphere( const Vector3& _center, const float _radius );

    //---------------------------------------------------------------------------------------------
    //! @brief      引数付きコンストラクタです.
    //!
    //! @param[in]      value       バウンディングボックスです.
    //---------------------------------------------------------------------------------------------
    BoundingSphere( const BoundingBox& value );

    //---------------------------------------------------------------------------------------------
    //! @brief      コピーコンストラクタです.
    //!
    //! @param[in]      value       コピー元の値です.
    //---------------------------------------------------------------------------------------------
    BoundingSphere( const BoundingSphere& value );

    //---------------------------------------------------------------------------------------------
    //! @brief      点を含むかどうか判定します.
    //!
    //! @param[in]      point       判定する点.
    //! @retval true    スフィア内です.
    //! @retval false   スフィア外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const Vector3& point ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      凸包を含むを判定します.
    //!
    //! @param[in]      pVertices       頂点配列.
    //! @param[in]      count           頂点数.
    //! @retval true    スフィア内です.
    //! @retval false   スフィア外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const Vector3* pVertices, const u32 count ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      バウンディングボックスを含むか判定します.
    //!
    //! @param[in]      box         判定するバウンディングボックス.
    //! @retval true    スフィア内です.
    //! @retval false   スフィア外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const BoundingBox& box ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      バウンディングスフィアを含むか判定します.
    //!
    //! @param[in]      sphere      判定するバウンディングスフィア.
    //! @retval true    スフィア内です.
    //! @retval false   スフィア外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const BoundingSphere& sphere ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      マージします.
    //!
    //! @param[in]      a           マージするバウンディングスフィア.
    //! @param[in]      b           マージするバウンディングスフィア.
    //! @return     2つのバウンディングスフィアをマージした結果を返却します.
    //---------------------------------------------------------------------------------------------
    static BoundingSphere Merge( const BoundingSphere& a, const BoundingSphere& b );
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// ViewFrustum
// ※ Game Programing Gems 5. "Improved Frustum Culling", pp.65-77 を参照.
///////////////////////////////////////////////////////////////////////////////////////////////////
class ViewFrustum
{
    //=============================================================================================
    // list of friend classes and methods.
    //=============================================================================================
    /* NOTHING */

public:
    //=============================================================================================
    // public variables.
    //=============================================================================================
    /* NOTHING */

    //=============================================================================================
    // public methods.
    //=============================================================================================

    //---------------------------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //---------------------------------------------------------------------------------------------
    ViewFrustum();

    //---------------------------------------------------------------------------------------------
    //! @brief      透視変換パラメータを設定します.
    //!
    //! @param[in]      fieldOfView     視野角.
    //! @param[in]      aspectRatio     アスペクト比.
    //! @param[in]      nearClip        ニアクリップ平面までの距離.
    //! @param[in]      farClip         ファークリップ平面までの距離.
    //---------------------------------------------------------------------------------------------
    void SetPerspective( 
        const float fieldOfView,
        const float aspectRatio,
        const float nearClip,
        const float farClip );

    //---------------------------------------------------------------------------------------------
    //! @brief      ビュー変換パラメータを設定します.
    //!
    //! @param[in]      position        視点位置.
    //! @param[in]      forward         基底ベクトル前方向.
    //! @param[in]      right           基底ベクトル右方向.
    //! @param[in]      upward          基底ベクトル上方向.
    //---------------------------------------------------------------------------------------------
    void SetView(
        const Vector3& position,
        const Vector3& forward,
        const Vector3& right,
        const Vector3& upward );

    //---------------------------------------------------------------------------------------------
    //! @brief      ビュー変換パラメータを設定します.
    //!
    //! @param[in]      position        視点位置です.
    //! @param[in]      target          注視点です.
    //! @param[in]      upward          上向きベクトルです.
    //---------------------------------------------------------------------------------------------
    void SetLookAt(
        const Vector3& position,
        const Vector3& target,
        const Vector3& upward );

    //---------------------------------------------------------------------------------------------
    //! @brief      ビュー変換パラメータを設定します.
    //!
    //! @param[in]      position        視点位置です.
    //! @param[in]      direction       視線ベクトルです.
    //! @param[in]      upward          上向きベクトルです.
    //---------------------------------------------------------------------------------------------
    void SetLookTo(
        const Vector3& position,
        const Vector3& direction,
        const Vector3& upward );

    //---------------------------------------------------------------------------------------------
    //! @brief      点を含むか判定します.
    //!
    //! @param[in]      point       点の位置座標.
    //! @retval true    錐台内です.
    //! @retval false   錐台外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const Vector3& point ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      凸包を含むか判定します.
    //!
    //! @param[in]      pVertices   頂点データ配列です.
    //! @param[in]      count       頂点数です.
    //! @retval true    錐台内です.
    //! @retval fasle   錐台外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const Vector3* pVertices, const u32 count ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      バウンディングスフィアを含むか判定します.
    //!
    //! @param[in]      sphere      判定するバウンディングスフィア.
    //! @retval true    錐台内です.
    //! @retval false   錐台外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const BoundingSphere& sphere ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      バウンディングボックスを含むか判定します.
    //!
    //! @param[in]      box         判定するバウンディングボックスです.
    //! @retval true    錐台内です.
    //! @retval false   錐台外です.
    //---------------------------------------------------------------------------------------------
    bool Contains( const BoundingBox& box ) const;

    //---------------------------------------------------------------------------------------------
    //! @brief      8角の頂点を取得します.
    //!
    //! @return     8角の頂点を返却します.
    //---------------------------------------------------------------------------------------------
    std::array<Vector3, 8> GetCorners() const;

private:
    //=============================================================================================
    // private variables.
    //=============================================================================================
    Vector3 m_Position;     //!< 視点位置です.
    Vector3 m_Forward;      //!< 基底ベクトル(前)
    Vector3 m_Right;        //!< 基底ベクトル(右)
    Vector3 m_Upward;       //!< 基底ベクトル(上)
    float     m_FactorR;      //!< rFactor
    float     m_FactorU;      //!< uFactor
    float     m_NearClip;     //!< ニアクリップ平面までの距離.
    float     m_FarClip;      //!< ファークリップ平面までの距離.
};

}// namespace asvk


//-------------------------------------------------------------------------------------------------
// Inline Files
//-------------------------------------------------------------------------------------------------
#include <detail/asvkGeometry.inl>
