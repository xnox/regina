
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
#import "SpreadHelper.h"
#import "SurfacesCoords.h"
#import "SurfacesViewController.h"
#import "TextHelper.h"
#import "MDSpreadViewClasses.h"
#import "snappea/nsnappeatriangulation.h"
#import "surfaces/nnormalsurfacelist.h"
#import "triangulation/ntriangulation.h"

#define KEY_SURFACES_COORDS_COMPACT @"SurfacesCoordsCompact"

static NSString *compactCellID = @"_ReginaCompactCell";
static NSString *regularCellID = @"_ReginaRegularCell";
static NSString *headerCellID = @"_ReginaHeaderCell";

static UIColor *headerBlack = [UIColor blackColor];
static UIColor *headerBdry = [UIColor colorWithRed:(0xB8 / 256.0) green:(0x86 / 256.0) blue:(0x0B / 256.0) alpha:1.0];

// TODO: On long press, view details (and cut/crush).

#define PROP_NONE 0
#define PROP_NAME 1
#define PROP_EULER 2
#define PROP_ORBL 3
#define PROP_SIDES 4
#define PROP_BDRY 5
#define PROP_LINK 6
#define PROP_TYPE 7

static NSArray* embProps = @[@PROP_EULER, @PROP_ORBL, @PROP_SIDES, @PROP_BDRY, @PROP_LINK];
static NSArray* nonEmbProps = @[@PROP_EULER, @PROP_BDRY, @PROP_LINK];

@interface SurfacesCoords () <MDSpreadViewDataSource, MDSpreadViewDelegate, PacketDelegate> {
    NSArray* widthProps;
    CGFloat widthCoord;
    CGFloat height;
    CGFloat widthHeader;
    regina::NormalCoords viewCoords;
}
@property (weak, nonatomic) IBOutlet UISwitch *compact;
@property (weak, nonatomic) IBOutlet UISegmentedControl *selectCoords;
@property (weak, nonatomic) IBOutlet MDSpreadView *grid;
@end

@implementation SurfacesCoords

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];

    self.compact.on = [[NSUserDefaults standardUserDefaults] boolForKey:KEY_SURFACES_COORDS_COMPACT];

    self.grid.defaultHeaderColumnCellClass = [RegularSpreadHeaderCellRight class];
    self.grid.defaultHeaderCornerCellClass = [RegularSpreadHeaderCellCentre class];
    // The column headers are managed manually, so that we can colour them if required.
    self.grid.dataSource = self;
    self.grid.delegate = self;
    self.grid.allowsRowHeaderSelection = YES;

    [self reloadPacket];
}

- (void)reloadPacket
{
    [super reloadPacket];

    if (self.packet->allowsAlmostNormal()) {
        [self.selectCoords setTitle:@"Quad-oct" forSegmentAtIndex:1];
    } else {
        [self.selectCoords setTitle:@"Quad" forSegmentAtIndex:1];
    }

    viewCoords = self.packet->coords();
    switch (viewCoords) {
        case regina::NS_STANDARD:
        case regina::NS_AN_STANDARD:
        case regina::NS_AN_LEGACY:
        case regina::NS_ORIENTED:
            self.selectCoords.selectedSegmentIndex = 0;
            break;
        case regina::NS_QUAD:
        case regina::NS_AN_QUAD_OCT:
        case regina::NS_ORIENTED_QUAD:
            self.selectCoords.selectedSegmentIndex = 1;
            break;
        case regina::NS_EDGE_WEIGHT:
        case regina::NS_TRIANGLE_ARCS:
        case regina::NS_ANGLE:
            // Should never occur.
            self.selectCoords.selectedSegmentIndex = 0;
            break;
    }

    [self initMetrics];
    [self.grid reloadData];
}

- (void)initMetrics
{
    NSMutableArray* w = [[NSMutableArray alloc] init];

    CGFloat tmp;
    for (NSNumber* prop in (self.packet->isEmbeddedOnly() ? embProps : nonEmbProps)) {
        switch (prop.intValue) {
            case PROP_NAME:
                tmp = [RegularSpreadViewCell cellSizeFor:@"Name"].width;
                break;
            case PROP_EULER:
                tmp = (self.compact.on ? [RegularSpreadViewCell cellSizeFor:@"-99"] : [RegularSpreadHeaderCell cellSizeFor:@"Euler"]).width;
                break;
            case PROP_ORBL:
                tmp = [RegularSpreadViewCell cellSizeFor:(self.compact.on ? @"✓" : @"Non-or.")].width;
                break;
            case PROP_SIDES:
                tmp = (self.compact.on ? [RegularSpreadViewCell cellSizeFor:@"2"] : [RegularSpreadHeaderCell cellSizeFor:@"Sides"]).width;
                break;
            case PROP_BDRY:
                if (self.packet->getTriangulation()->isClosed())
                    tmp = (self.compact.on ? [RegularSpreadViewCell cellSizeFor:@"—"] : [RegularSpreadHeaderCell cellSizeFor:@"Bdry"]).width;
                else if (! self.packet->allowsSpun())
                    tmp = [RegularSpreadViewCell cellSizeFor:@"Real"].width;
                else if (! dynamic_cast<regina::NSnapPeaTriangulation*>(self.packet->getTriangulation()))
                    tmp = [RegularSpreadViewCell cellSizeFor:@"Spun"].width;
                else
                    tmp = [RegularSpreadViewCell cellSizeFor:@"Spun (99, 99)"].width;
                break;
            case PROP_LINK:
                tmp = [RegularSpreadViewCell cellSizeFor:@"Edge 99"].width;
                break;
            case PROP_TYPE:
                tmp = [RegularSpreadViewCell cellSizeFor:@"Splitting"].width;
                break;
        }
        [w addObject:[NSNumber numberWithFloat:tmp]];
    }

    if (self.packet->allowsAlmostNormal()) {
        tmp = [RegularSpreadViewCell cellSizeFor:@"K99: 99/99 (99 octs)"].width;
        [w addObject:[NSNumber numberWithFloat:tmp]];
    }

    widthProps = w;

    widthHeader = [RegularSpreadHeaderCell cellSizeFor:[NSString stringWithFormat:@"%ld.", self.packet->getNumberOfSurfaces() - 1]].width;

    if (self.compact.on) {
        CGSize s = [CompactSpreadViewCell cellSizeFor:@"g00"];
        widthCoord = s.width;
        height = s.height;
    } else {
        NSString* longestColumnName = [Coordinates longestColumnName:viewCoords tri:self.packet->getTriangulation()];
        if (longestColumnName.length < 2)
            longestColumnName = @"99"; // Ah, edge weight space.
        widthCoord = [RegularSpreadHeaderCell cellSizeFor:longestColumnName].width;
        height = [RegularSpreadViewCell cellSizeFor:@"g0"].height;
    }
}

- (IBAction)compactChanged:(id)sender {
    [[NSUserDefaults standardUserDefaults] setBool:self.compact.on forKey:KEY_SURFACES_COORDS_COMPACT];

    [self initMetrics];
    [self.grid reloadData];
}

- (IBAction)coordsChanged:(id)sender {
    switch (self.selectCoords.selectedSegmentIndex) {
        case 0:
            viewCoords = (self.packet->allowsAlmostNormal() ? regina::NS_AN_STANDARD : regina::NS_STANDARD);
            break;
        case 1:
            viewCoords = (self.packet->allowsAlmostNormal() ? regina::NS_AN_QUAD_OCT : regina::NS_QUAD);
            break;
        case 2:
            viewCoords = regina::NS_EDGE_WEIGHT;
            break;
        case 3:
            viewCoords = regina::NS_FACE_ARCS;
            break;
    }

    [self initMetrics];
    [self.grid reloadData];
}

#pragma mark - MDSpreadView data source

- (NSInteger)spreadView:(MDSpreadView *)aSpreadView numberOfColumnsInSection:(NSInteger)section
{
    return (self.packet->isEmbeddedOnly() ? embProps.count : nonEmbProps.count) +
           (self.packet->allowsAlmostNormal() ? 1 : 0) +
           [Coordinates numColumns:viewCoords tri:self.packet->getTriangulation()];
}

- (NSInteger)spreadView:(MDSpreadView *)aSpreadView numberOfRowsInSection:(NSInteger)section
{
    return self.packet->getNumberOfSurfaces();
}

- (id)spreadView:(MDSpreadView *)aSpreadView titleForHeaderInColumnSection:(NSInteger)section forRowAtIndexPath:(MDIndexPath *)rowPath
{
    return [NSString stringWithFormat:@"%ld.", (long)rowPath.row];
}

- (MDSpreadViewCell *)spreadView:(MDSpreadView *)aSpreadView cellForHeaderInRowSection:(NSInteger)section forColumnAtIndexPath:(MDIndexPath *)columnPath
{
    MDSpreadViewCell* cell = [aSpreadView dequeueReusableCellWithIdentifier:headerCellID];
    if (! cell)
        cell = [[RegularSpreadHeaderCellCentre alloc] initWithStyle:MDSpreadViewHeaderCellStyleRow reuseIdentifier:headerCellID];

    cell.textLabel.textColor = headerBlack;

    if (self.compact.on) {
        cell.textLabel.text = @"";
        return cell;
    }

    int prop = PROP_NONE;
    int coord = columnPath.column;

    if (self.packet->isEmbeddedOnly()) {
        if (coord < embProps.count)
            prop = [embProps[coord] intValue];
        else
            coord -= embProps.count;
    } else {
        if (coord < nonEmbProps.count)
            prop = [nonEmbProps[coord] intValue];
        else
            coord -= nonEmbProps.count;
    }

    switch (prop) {
        case PROP_NONE: break;
        case PROP_NAME: cell.textLabel.text = @"Name"; return cell;
        case PROP_EULER: cell.textLabel.text = @"Euler"; return cell;
        case PROP_ORBL: cell.textLabel.text = @"Orient"; return cell;
        case PROP_SIDES: cell.textLabel.text = @"Sides"; return cell;
        case PROP_BDRY: cell.textLabel.text = @"Bdry"; return cell;
        case PROP_LINK: cell.textLabel.text = @"Link"; return cell;
        case PROP_TYPE: cell.textLabel.text = @"Type"; return cell;
    }

    if (self.packet->allowsAlmostNormal()) {
        if (coord == 0) {
            cell.textLabel.text = @"Octagon";
            return cell;
        } else
            --coord;
    }

    cell.textLabel.text = [Coordinates columnName:viewCoords
                                       whichCoord:coord
                                              tri:self.packet->getTriangulation()];

    if ((viewCoords == regina::NS_EDGE_WEIGHT && self.packet->getTriangulation()->getEdge(coord)->isBoundary()) ||
            (viewCoords == regina::NS_TRIANGLE_ARCS && self.packet->getTriangulation()->getTriangle(coord / 3)->isBoundary()))
        cell.textLabel.textColor = headerBdry;

    return cell;
}

- (MDSpreadViewCell *)spreadView:(MDSpreadView *)aSpreadView cellForRowAtIndexPath:(MDIndexPath *)rowPath forColumnAtIndexPath:(MDIndexPath *)columnPath
{
    MDSpreadViewCell* cell;
    if (self.compact.on) {
        cell = [aSpreadView dequeueReusableCellWithIdentifier:compactCellID];
        if (! cell)
            cell = [[CompactSpreadViewCell alloc] initWithReuseIdentifier:compactCellID];
    } else {
        cell = [aSpreadView dequeueReusableCellWithIdentifier:regularCellID];
        if (! cell)
            cell = [[RegularSpreadViewCell alloc] initWithReuseIdentifier:regularCellID];
    }

    const regina::NNormalSurface* s = self.packet->getSurface(rowPath.row);

    int prop = PROP_NONE;
    int coord = columnPath.column;

    if (self.packet->isEmbeddedOnly()) {
        if (coord < embProps.count)
            prop = [embProps[coord] intValue];
        else
            coord -= embProps.count;
    } else {
        if (coord < nonEmbProps.count)
            prop = [nonEmbProps[coord] intValue];
        else
            coord -= nonEmbProps.count;
    }

    switch (prop) {
        case PROP_NONE:
            break;
        case PROP_NAME:
            cell.textLabel.text = @(s->getName().c_str());
            cell.textLabel.textAlignment = NSTextAlignmentLeft;
            return cell;
        case PROP_EULER:
            if (s->isCompact())
                cell.textLabel.text = @(s->getEulerChar().stringValue().c_str());
            else
                cell.textLabel.text = @"";
            cell.textLabel.textAlignment = NSTextAlignmentRight;
            return cell;
        case PROP_ORBL:
            if (s->isCompact())
                cell.textLabel.attributedText = [TextHelper yesNoString:s->isOrientable()
                                                                    yes:@"✓"
                                                                     no:(self.compact.on ? @"✕" : @"Non-or.")];
            else
                cell.textLabel.text = @"";
            cell.textLabel.textAlignment = NSTextAlignmentCenter;
            return cell;
        case PROP_SIDES:
            if (s->isCompact())
                cell.textLabel.attributedText = [TextHelper yesNoString:s->isTwoSided() yes:@"2" no:@"1"];
            else
                cell.textLabel.text = @"";
            cell.textLabel.textAlignment = NSTextAlignmentCenter;
            return cell;
        case PROP_BDRY:
            if (! s->isCompact()) {
                regina::NMatrixInt* slopes = s->boundaryIntersections();
                if (slopes) {
                    NSMutableString* bdry = [NSMutableString stringWithString:@"Spun:"];
                    // Display each boundary slope as (nu(L), -nu(M)).
                    for (unsigned i = 0; i < slopes->rows(); ++i)
                        [bdry appendFormat:@" (%s, %s)",
                         slopes->entry(i,1).stringValue().c_str(),
                         (-slopes->entry(i,0)).stringValue().c_str()];
                    cell.textLabel.attributedText = [TextHelper markedString:bdry];
                } else
                    cell.textLabel.attributedText = [TextHelper markedString:@"Spun"];
            } else if (s->hasRealBoundary())
                cell.textLabel.attributedText = [TextHelper yesNoString:@"Real" yesNo:NO];
            else
                cell.textLabel.attributedText = [TextHelper yesNoString:@"—" yesNo:YES];
            cell.textLabel.textAlignment = NSTextAlignmentLeft;
            return cell;
        case PROP_LINK:
        {
            const regina::NVertex* v;
            std::pair<const regina::NEdge*, const regina::NEdge*> e;

            if ((v = s->isVertexLink()))
                cell.textLabel.text = [NSString stringWithFormat:@"Vertex %ld",
                                       self.packet->getTriangulation()->vertexIndex(v)];
            else if ((e = s->isThinEdgeLink()).first) {
                if (e.second)
                    cell.textLabel.text = [NSString stringWithFormat:@"Edges %ld, %ld",
                                           self.packet->getTriangulation()->edgeIndex(e.first),
                                           self.packet->getTriangulation()->edgeIndex(e.second)];
                else
                    cell.textLabel.text = [NSString stringWithFormat:@"Edge %ld",
                                           self.packet->getTriangulation()->edgeIndex(e.first)];
            } else
                cell.textLabel.text = @"";
            cell.textLabel.textAlignment = NSTextAlignmentLeft;
            return cell;
        }
        case PROP_TYPE:
        {
            regina::NLargeInteger tot;
            if (s->isSplitting())
                cell.textLabel.text = @"Splitting";
            else if (! (tot = s->isCentral()).isZero())
                cell.textLabel.text = [NSString stringWithFormat:@"Central (%ld)", tot.longValue()];
            else
                cell.textLabel.text = @"";
            cell.textLabel.textAlignment = NSTextAlignmentLeft;
            return cell;
        }
    }

    if (self.packet->allowsAlmostNormal()) {
        if (coord == 0) {
            regina::NDiscType oct = s->getOctPosition();
            if (oct == regina::NDiscType::NONE) {
                cell.textLabel.text = @"";
                return cell;
            }

            regina::NLargeInteger tot = s->getOctCoord(oct.tetIndex, oct.type);
            if (tot == 1) {
                cell.textLabel.attributedText = [TextHelper yesNoString:[NSString stringWithFormat:@"K%ld: %s (1 oct)",
                                                                         oct.tetIndex,
                                                                         regina::vertexSplitString[oct.type]]
                                                                  yesNo:YES];
            } else {
                cell.textLabel.attributedText = [TextHelper yesNoString:[NSString stringWithFormat:@"K%ld: %s (%s octs)",
                                                                         oct.tetIndex,
                                                                         regina::vertexSplitString[oct.type],
                                                                         tot.stringValue().c_str()]
                                                                  yesNo:NO];
            }
            cell.textLabel.textAlignment = NSTextAlignmentLeft;
            return cell;
        } else
            --coord;
    }

    regina::NLargeInteger val = [Coordinates getCoordinate:viewCoords surface:*s whichCoord:coord];
    if (val.isZero())
        cell.textLabel.text = @"";
    else if (val.isInfinite())
        cell.textLabel.text = @"∞";
    else
        cell.textLabel.text = @(val.stringValue().c_str());

    cell.textLabel.textAlignment = NSTextAlignmentRight;
    return cell;
}

#pragma mark - MDSpreadView delegate

- (CGFloat)spreadView:(MDSpreadView *)aSpreadView widthForColumnHeaderInSection:(NSInteger)columnSection
{
    return widthHeader;
}

- (CGFloat)spreadView:(MDSpreadView *)aSpreadView widthForColumnAtIndexPath:(MDIndexPath *)indexPath
{
    if (indexPath.column < widthProps.count)
        return [widthProps[indexPath.column] floatValue];
    else
        return widthCoord;
}

- (CGFloat)spreadView:(MDSpreadView *)aSpreadView heightForRowHeaderInSection:(NSInteger)rowSection
{
    return (self.compact.on ? 1 : height);
}

- (CGFloat)spreadView:(MDSpreadView *)aSpreadView heightForRowAtIndexPath:(MDIndexPath *)indexPath
{
    return height;
}

@end