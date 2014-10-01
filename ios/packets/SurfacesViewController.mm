
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  iOS User Interface                                                    *
 *                                                                        *
 *  Copyright (c) 1999-2013, Ben Burton                                   *
 *  For further details contact Ben Burton (bab@debian.org).              *
 *                                                                        *
 *  This program is free software; you can redistribute it and/or         *
 *  modify it under the terms of the GNU General Public License as        *
 *  published by the Free Software Foundation; either version 2 of the    *
 *  License, or (at your option) any later version.                       *
 *                                                                        *
 *  As an exception, when this program is distributed through (i) the     *
 *  App Store by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or     *
 *  (iii) Google Play by Google Inc., then that store may impose any      *
 *  digital rights management, device limits and/or redistribution        *
 *  restrictions that are required by its terms of service.               *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful, but   *
 *  WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *  General Public License for more details.                              *
 *                                                                        *
 *  You should have received a copy of the GNU General Public             *
 *  License along with this program; if not, write to the Free            *
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,       *
 *  MA 02110-1301, USA.                                                   *
 *                                                                        *
 **************************************************************************/

#import "Coordinates.h"
#import "SurfacesViewController.h"
#import "surfaces/nnormalsurfacelist.h"
#import "triangulation/ntriangulation.h"

#pragma mark - Surfaces view controller

@implementation SurfacesViewController

// TODO: Listen for renames on triangulation.

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self setSelectedImages:@[@"Tab-Summary-Bold",
                              @"Tab-Coords-Bold",
                              @"Tab-Compat-Bold",
                              @"Tab-Matching-Bold"]];
    [self registerDefaultKey:@"ViewSurfacesTab"];
}

- (NSString *)packetActionIcon
{
    return @"NavBar-Surfaces";
}

- (void)packetActionPressed
{
    // TODO: Implement actions.
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"To Be Implemented"
                                                    message:nil
                                                   delegate:nil
                                          cancelButtonTitle:@"Close"
                                          otherButtonTitles:nil];
    [alert show];
}

- (void)updateHeader:(UILabel *)summary coords:(UILabel *)coords tri:(UIButton *)tri
{
    regina::NormalList which = self.packet->which();

    NSString *sEmb, *sType;

    if (which.has(regina::NS_EMBEDDED_ONLY))
        sEmb = @"embedded";
    else if (which.has(regina::NS_IMMERSED_SINGULAR))
        sEmb = @"embedded / immersed / singular";
    else
        sEmb = @"unknown";

    if (which.has(regina::NS_VERTEX))
        sType = @"vertex";
    else if (which.has(regina::NS_FUNDAMENTAL))
        sType = @"fundamental";
    else if (which.has(regina::NS_CUSTOM))
        sType = @"custom";
    else if (which.has(regina::NS_LEGACY))
        sType = @"legacy";
    else
        sType = @"unknown";

    if (self.packet->getNumberOfSurfaces() == 0)
        summary.text = [NSString stringWithFormat:@"No %@, %@ surfaces", sType, sEmb];
    else if (self.packet->getNumberOfSurfaces() == 1)
        summary.text = [NSString stringWithFormat:@"1 %@, %@ surface", sType, sEmb];
    else
        summary.text = [NSString stringWithFormat:@"%ld %@, %@ surfaces",
                        self.packet->getNumberOfSurfaces(), sType, sEmb];

    coords.text = [NSString stringWithFormat:@"Enumerated in %@ coordinates", [Coordinates name:self.packet->coords() capitalise:NO]];
    [self updateTriangulationButton:tri];
}

- (void)updateTriangulationButton:(UIButton*)button
{
    regina::NPacket* tri = self.packet->getTriangulation();
    NSString* triName = [NSString stringWithUTF8String:tri->getPacketLabel().c_str()];
    if (triName.length == 0)
        triName = @"(Unnamed)";
    [button setTitle:triName forState:UIControlStateNormal];
}

@end

#pragma mark - Surfaces tab

@interface SurfacesTab () <PacketDelegate> {
    PacketListenerIOS* _triListener;
}
@property (weak, nonatomic) IBOutlet UILabel *header;
@property (weak, nonatomic) IBOutlet UILabel *coords;
@property (weak, nonatomic) IBOutlet UIButton *tri;
@end

@implementation SurfacesTab

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.viewer = static_cast<SurfacesViewController*>(self.parentViewController);
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];

    self.packet = self.viewer.packet;

    [_triListener permanentlyUnlisten];
    _triListener = [PacketListenerIOS listenerWithPacket:self.packet->getTriangulation() delegate:self listenChildren:NO];
}

- (void)dealloc
{
    [_triListener permanentlyUnlisten];
}

- (void)reloadPacket
{
    [self.viewer updateHeader:self.header coords:self.coords tri:self.tri];
}

- (void)packetWasRenamed:(regina::NPacket *)packet
{
    if (packet == self.packet->getTriangulation())
        [self.viewer updateTriangulationButton:self.tri];
}

@end
