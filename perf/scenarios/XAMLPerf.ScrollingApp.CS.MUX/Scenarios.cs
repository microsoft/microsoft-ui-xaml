using Interactions;
using System.Threading.Tasks;

namespace XAMLPerf.ScrollingApp.CS.MUX
{
    public class ScrollingScenario
    {
        IControlInteractionModel interactionModel;

        const int ActivateCount = 3;
        const uint PostActivateDelayMs = 500;

        const int ItemScrollCount = 5;
        const uint PostItemScrollDelayMs = 400;

        const int PageScrollCount = 10;
        const uint PostPageScrollDelayMs = 400;

        public ScrollingScenario(IControlInteractionModel interactionModel)
        {
            this.interactionModel = interactionModel;
        }

        public async Task Execute()
        {
            // 1.0 Prepare

            await interactionModel.BuildPrepareStateAction()();

            // 1.1 Activate

            await Common.Repeat(
                ActivateCount,
                PostActivateDelayMs,
                interactionModel.BuildInvokeAction());

            // 1.2 From the beginning

            await interactionModel.BuildGoToBeginningAction()();

            // 1.2.1 Scroll by item

            await Common.Repeat(
                ItemScrollCount,
                PostItemScrollDelayMs,
                interactionModel.BuildScrollLineForwardAction());

            await Common.Repeat(
                ItemScrollCount,
                PostItemScrollDelayMs,
                interactionModel.BuildScrollLineBackAction());

            // 1.2.2 Scroll by page

            await Common.Repeat(
                PageScrollCount,
                PostPageScrollDelayMs,
                interactionModel.BuildScrollPageForwardAction());

            await Common.Repeat(
                PageScrollCount,
                PostPageScrollDelayMs,
                interactionModel.BuildScrollPageBackAction());

            // 1.3 From the end

            await interactionModel.BuildGoToEndAction()();

            // 1.3.1 Scroll by item

            await Common.Repeat(
                ItemScrollCount,
                PostItemScrollDelayMs,
                interactionModel.BuildScrollLineBackAction());

            await Common.Repeat(
                ItemScrollCount,
                PostItemScrollDelayMs,
                interactionModel.BuildScrollLineForwardAction());

            // 1.3.2 Scroll by page

            await Common.Repeat(
                PageScrollCount,
                PostPageScrollDelayMs,
                interactionModel.BuildScrollPageBackAction());

            await Common.Repeat(
                PageScrollCount,
                PostPageScrollDelayMs,
                interactionModel.BuildScrollPageForwardAction());
        }
    }
}
