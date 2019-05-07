//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

import QtQuick 2.11
import QtQuick.Controls 2.4
import JASP.Theme 1.0

Rectangle
{
	id							: ribbonButton
	width						: implicitWidth
	height						: implicitHeight
	implicitHeight				: Theme.ribbonButtonHeight * 0.6
	implicitWidth				: implicitHeight
	// radius					: 5
	color						: mice.pressed ? Theme.grayLighter : Theme.uiBackground

	property bool	hamburger:	true
	property bool	showArrow:	false
	property string	toolTip:	""

	ToolTip.text:				toolTip
	ToolTip.timeout:			Theme.toolTipTimeout
	ToolTip.delay:				Theme.toolTipDelay / 5
	ToolTip.toolTip.font:		Theme.font
	ToolTip.visible:			toolTip !== "" && mice.containsMouse
	ToolTip.toolTip.background: Rectangle { color:	Theme.tooltipBackgroundColor }

	signal clicked

	Item
	{
		id:					hamburgerArrow
		anchors.centerIn:	parent
		width:				hamburgerArrow.barWidth//parent.width	- (2 * Theme.ribbonButtonPadding)
		height:				baseHeight - 20
		scale:				baseScale * (mice.containsMouse && !mice.pressed ? Theme.ribbonScaleHovered : 1)


		property real	baseScale:		0.7 * (parent.height / baseHeight)//Ok changing height doesnt work well for this component so I just scale it when necessary!

		property real	baseHeight:		80
		property real	barThickness:	8 //(Theme.ribbonButtonHeight (2 * Theme.ribbonButtonPadding)) / 7
		property real	barRadius:		barThickness
		property real	barWidth:		baseHeight / 2
		property color	barColor:		Theme.grayDarker

		Item
		{
			id:					topBar
			anchors.centerIn:	parent
			height:				hamburgerArrow.barThickness
			width:				parent.width

			Rectangle
			{
				anchors.centerIn:	ribbonButton.showArrow  ||  ribbonButton.hamburger ? undefined		: parent
				anchors.left:		!ribbonButton.showArrow || !ribbonButton.hamburger ? undefined	: parent.left
				anchors.right:		!ribbonButton.showArrow ||  ribbonButton.hamburger ? undefined	: parent.right
				transformOrigin:	!ribbonButton.showArrow ? Item.Center	: ribbonButton.hamburger ? Item.Left	: Item.Right

				y:					ribbonButton.showArrow ? height * 0.25 : ribbonButton.hamburger ? hamburgerArrow.barThickness * 2 : 0

				height:				parent.height
				width:				!ribbonButton.showArrow ? parent.width : parent.width / 1.25
				rotation:			!ribbonButton.showArrow ?
										(ribbonButton.hamburger ?   0 : 90) :
										(ribbonButton.hamburger ? -45 : 45)
				radius:				hamburgerArrow.barRadius
				color:				hamburgerArrow.barColor
			}
		}

		Rectangle
		{
			id:		middleBar
			height:	hamburgerArrow.barThickness
			width:	parent.width
			radius:	hamburgerArrow.barRadius
			color:	hamburgerArrow.barColor

			anchors.centerIn:	parent
		}

		Item
		{
			id:					bottomBar
			anchors.centerIn:	parent
			height:				hamburgerArrow.barThickness
			width:				parent.width

			Rectangle
			{
				anchors.centerIn:	ribbonButton.showArrow ||   ribbonButton.hamburger	? undefined		: parent
				anchors.left:		!ribbonButton.showArrow || !ribbonButton.hamburger	? undefined		: parent.left
				anchors.right:		!ribbonButton.showArrow ||  ribbonButton.hamburger	? undefined		: parent.right
				transformOrigin:	!ribbonButton.showArrow								? Item.Center	: ribbonButton.hamburger ? Item.Left	: Item.Right

				y:					ribbonButton.showArrow ? -height * 0.25 : ribbonButton.hamburger ? -hamburgerArrow.barThickness * 2 : 0

				height:				parent.height
				width:				!ribbonButton.showArrow ? parent.width : parent.width / 1.25
				rotation:			!ribbonButton.showArrow ? 0 : ribbonButton.hamburger ? 45 : -45
				radius:				hamburgerArrow.barRadius
				color:				hamburgerArrow.barColor

			}
		}
	}

	MouseArea
	{
		id						: mice
		anchors.fill			: parent
		hoverEnabled			: true
		acceptedButtons			: Qt.LeftButton
		onClicked				: ribbonButton.clicked(); //{ itsHoverTime.stop(); ribbonButton.clickWhenAllowed();  }
		cursorShape				: Qt.PointingHandCursor
		//onContainsMouseChanged	: if(containsMouse) itsHoverTime.start(); else itsHoverTime.stop();
	}

	/* Hmm, hover works a bit weird, disabling it.
	property bool clickingAllowed: true

	function clickWhenAllowed()
	{
		if(clickingAllowed)
		{
			clickingAllowed = false;
			ribbonButton.clicked();
			blockDoubleClicksTimer.start();
		}
	}

	Timer
	{
		id:				itsHoverTime
		interval:		Theme.hoverTime * 2
		repeat:			false
		running:		false

		onTriggered:	ribbonButton.clickWhenAllowed();
	}

	Timer
	{
		id:				blockDoubleClicksTimer
		interval:		Theme.fileMenuSlideDuration * 4
		repeat:			false
		running:		false

		onTriggered:	ribbonButton.clickingAllowed = true;
	}*/
}
