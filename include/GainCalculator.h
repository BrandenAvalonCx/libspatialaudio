/*############################################################################*/
/*#                                                                          #*/
/*#  A gain calculator with ADM metadata with speaker or HOA output			 #*/
/*#                                                                          #*/
/*#  Filename:      GainCalculator.h	                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          28/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "Coordinates.h"
#include "Tools.h"
#include "AdmMetadata.h"
#include "PolarExtent.h"
#include "AllocentricExtent.h"
#include "AmbisonicEncoder.h"
#include "Screen.h"

namespace admrender {

	/** A class to apply ChannelLocking as described in Rec. ITU-R BS.2127-1 sec. 7.3.6 pg44. */
	class ChannelLockHandler
	{
	public:
		ChannelLockHandler(const Layout& layout);
		~ChannelLockHandler();

		/** If the Object has a channelLock set then determines the new direction of the object within an optional distance.
		 *  Otherwise the original position is returned.
		 *
		 * @param channelLock	Optional channel lock. If not set then original position returned. If its max distance is set then only speakers closer than this will be considered for locking.
		 * @param position		The position to be processed.
		 * @param exlcuded		Flag if a loudspeaker is to be excluded from channel locking. If it has size = 0 then all loudspeaker are considered.
		 * @return				The processed position. If position is not within the specified distance of for ChannelLocking then the original position is returned.
		 */
		CartesianPosition handle(const Optional<ChannelLock>& channelLock, CartesianPosition position, const std::vector<bool>& exlcuded);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;

		std::vector<double> m_distance;
		std::vector<unsigned int> m_closestSpeakersInd;
		std::vector<unsigned int> m_equalDistanceSpeakers;
		std::vector<std::vector<double>> m_tuple;
		std::vector<std::vector<double>> m_tupleSorted;
		int m_activeTuples = 0;

		/** Pure virtual function to override that defines how the distance between a position and a speaker are calculated. */
		virtual double calculateDistance(const CartesianPosition& srcPos, const CartesianPosition& spkPos) = 0;

	protected:
		// Speaker positions: normalised for polar processing or else allocentric loudspeaker coordinates
		std::vector<CartesianPosition> m_spkPos;
	};

	class PolarChannelLockHandler : public ChannelLockHandler
	{
	public:
		PolarChannelLockHandler(const Layout& layout);
		~PolarChannelLockHandler();

	private:
		double calculateDistance(const CartesianPosition& srcPos, const CartesianPosition& spkPos) override;
	};
	class AlloChannelLockHandler : public ChannelLockHandler
	{
	public:
		AlloChannelLockHandler(const Layout& layout);
		~AlloChannelLockHandler();

	private:
		double calculateDistance(const CartesianPosition& srcPos, const CartesianPosition& spkPos) override;
	};

	/** A class to handle zone exclusion as described in Rec. ITU-R BS.2127-1 sec. 7.3.12 pg. 60. */
	class ZoneExclusionHandler
	{
	public:
		ZoneExclusionHandler(const Layout& layout);
		~ZoneExclusionHandler();

		/** Return the exlcusion flags for all of the loudspeakers for cartesian/allocentric panning.
		 * @param exclusionZones	Vector of the exclusion zones.
		 * @param excluded			Vector indicating which loudspeakers are to be excluded.
		 */
		void getCartesianExcluded(const std::vector<ExclusionZone>& exclusionZones, std::vector<bool>& excluded);

		/** Calculate the gain vector once the appropriate loudspeakers have been exlcuded. The gains are replaced with the processed version.
		 * @param exclusionZones	Vector of all exclusion zones.
		 * @param gainsInOut		Input panning gains which are replaced by the processed ones.
		 */
		void handle(const std::vector<ExclusionZone>& exclusionZones, std::vector<double>& gainsInOut);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;
		std::vector<std::vector<std::set<unsigned int>>> m_downmixMapping;
		std::vector<std::vector<unsigned int>> m_downmixMatrix;

		// Conversion of the nominal polar positions to cartesian
		std::vector<CartesianPosition> m_cartesianPositions;

		// Indices of each of the loudspeaker in each row
		std::vector<std::vector<unsigned int>> m_rowInds;

		/** Get the excluded loudspeakers based on whether or not they are inside a polar or cartesian exclusion zone.
		 * @param exclusionZones	Vector of the exclusion zones.
		 * @param excluded			Vector indicating which loudspeakers are to be excluded.
		 */
		void getExcluded(const std::vector<ExclusionZone>& exclusionZones, std::vector<bool>& excluded);

		/** Count the number of excluded loudspeakers.
		 * @param exlcuded	Vector indicating if a loudspeaker has been excluded.
		 * @return			Total number of excluded loudspeakers.
		 */
		int getNumExcluded(const std::vector<bool>& exlcuded);

		/** Get the layer priority based on the input and output channel types.
		 * @param inputChannelName	Name of the input channel.
		 * @param outputChannelName	Name of the output channel.
		 * @return					Priority of the layer.
		 */
		int GetLayerPriority(const std::string& inputChannelName, const std::string& outputChannelName);

		// Downmix matrix
		std::vector<std::vector<double>> m_D;
		// Vector holding the exclusion state of each channel
		std::vector<bool> m_isExcluded;
		// Temp vector of the gains
		std::vector<double> m_gainsTmp;

		std::vector<unsigned int> m_notExcludedElements;
		std::vector<unsigned int> m_setElements;

	};

	/** The main ADM gain calculator class which processes metadata to calculate direct and diffuse gains. */
	class CGainCalculator
	{
	public:
		CGainCalculator(Layout outputLayoutNoLFE);
		~CGainCalculator();

		/** Calculate the panning (loudspeaker or HOA) gains to apply to a
		 *	mono signal for spatialisation based on the input metadata.
		 *
		 * @param metadata		Object metadata to be used to calculate the gains.
		 * @param directGains	Output vector of direct gains corresponding to the supplied metadata.
		 * @param diffuseGains	Output vector of diffuse gains corresponding to the supplied metadata.
		 */
		void CalculateGains(const ObjectMetadata& metadata, std::vector<double>& directGains, std::vector<double>& diffuseGains);

	private:
		// The output layout
		Layout m_outputLayout;
		// number of output channels
		unsigned int m_nCh;
		// number of output channels excluding LFE channels
		unsigned int m_nChNoLFE;

		// The cartesian/allocentric positions for the speakers, if a valid array is selected
		std::vector<CartesianPosition> m_cartPositions;

		CPointSourcePannerGainCalc m_pspGainCalculator;
		CPolarExtentHandler m_extentPanner;
		CAmbisonicPolarExtentHandler m_ambiExtentPanner;

		CAllocentricPannerGainCalc m_alloGainCalculator;
		CAllocentricExtent m_alloExtentPanner;

		CScreenScaleHandler m_screenScale;
		CScreenEdgeLock m_screenEdgeLock;

		PolarChannelLockHandler m_polarChannelLockHandler;
		AlloChannelLockHandler m_alloChannelLockHandler;
		ZoneExclusionHandler m_zoneExclusionHandler;

		std::vector<double> m_gains;

		std::vector<CartesianPosition> m_divergedPos;
		std::vector<double> m_divergedGains;
		std::vector<std::vector<double>> m_gainsForEachPos;

		// Vector of excluded loudspeakers for cartesian processing
		std::vector<bool> m_excluded;

		// Flag if the layout supports cartesian/allocentric panning. If not, convert metadata to polar.
		bool m_cartesianLayout = false;
		ObjectMetadata m_objMetadata;

		/** Get the diverged source positions and directions. See Rec. ITU-R BS.2127-1 sec. 7.3.7 pg. 45.
		* @param objectDivergence		Optional object divergence. If not set then returns original position with a single unity gain.
		* @param position				The position of the source.
		* @param cartesian				Flag if Cartesian processing option is to be used.
		* @param divergedPos			Output of the diverged position(s) with size 1 or 3.
		* @param divergedGains			Output of the diverged gain(s) with size 1 or 3.
		*/
		void divergedPositionsAndGains(const Optional<ObjectDivergence>& objectDivergence, CartesianPosition position, bool cartesian, std::vector<CartesianPosition>& divergedPos, std::vector<double>& divergedGains);

		/** Insert LFE entry with 0 gain in to a set of gains based on the supplied layout.
		 * @param layout		Output layout.
		 * @param gainsNoLFE	Vector of gains without LFE.
		 * @param gainsWithLFE	Return vector of gains with LFE inserted.
		 */
		void insertLFE(const Layout& layout, const std::vector<double>& gainsNoLFE, std::vector<double>& gainsWithLFE);
	};
} // namespace admrenderer
